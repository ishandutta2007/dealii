// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2018 - 2024 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------


/**
 * @defgroup matrixfree Matrix-free infrastructure
 *
 * This group describes the matrix-free infrastructure in deal.II.
 * An outline of how the primary groups of classes in deal.II interact with the
 * matrix-free infrastructure is given by the following clickable graph,
 * with a more detailed description below:
 *
 * @dot
digraph G
{
  graph[rankdir="TB",bgcolor="transparent"];
  node [shape=box,fontname="FreeSans",fontsize=15,
        height=0.2,width=0.4,
        color="black", fillcolor="white", style="filled"];
  edge [color="black", weight=10];
  subgraph base {
    rank="same";
  tria       [label="Triangulation",    URL="\ref grid"];
  fe         [label="FiniteElement",    URL="\ref feall"];
  mapping    [label="Mapping",          URL="\ref mapping"];
  quadrature [label="Quadrature",       URL="\ref Quadrature"];
  }
  dh         [label="DoFHandler",       URL="\ref dofs"];
  simd     [label="SIMD", fontname="FreeSans",fontsize=12,
            height=0.2,width=0.4,
            color="gray", fontcolor="gray", fillcolor="white", style="filled"];
  fevalues [label="FEEvaluation", fillcolor="deepskyblue"];
  mf       [label="MatrixFree loops", fillcolor="deepskyblue"];
  portable     [label="Portable",     URL="\ref Portable", fontname="FreeSans",fontsize=12,
            height=0.2,width=0.4,
            color="gray", fontcolor="gray", fillcolor="white", style="filled"];
  tbb      [label="TBB", fontname="FreeSans",fontsize=12,
            height=0.2,width=0.4,
            color="gray", fontcolor="gray", fillcolor="white", style="filled"];
{rank=same
  simd -> fevalues        [dir="none", color="transparent"];
  fevalues -> mf          [dir="none", color="transparent"];
  mf -> portable              [dir="none", color="transparent"];
  portable -> tbb             [dir="none", color="transparent"];
}
  subgraph sol {
    rank="same";
    solvers [label="Solvers",   URL="\ref Solvers", fillcolor="deepskyblue"];
    gmg     [label="Geometric Multigrid", fontname="FreeSans",fontsize=12,
             height=0.2,width=0.4,
             color="black", fontcolor="black", fillcolor="white", style="dashed"];
  }
  output     [label="Graphical output", URL="\ref output"];
  manifold   [label="Manifold",         URL="\ref manifold"];
  tria -> dh              [color="black",style="solid"];
  fe -> dh                [color="black",style="solid"];
  fe -> fevalues          [color="black",style="solid"];
  mapping -> fevalues     [color="black",style="solid"];
  quadrature -> fevalues  [color="black",style="solid"];
  dh -> mf                [color="black",style="solid"];
  mf -> systems           [color="black",style="solid"];
  fevalues -> systems     [color="black",style="solid"];
  systems -> solvers      [color="black",style="solid"];
  solvers -> output       [color="black",style="solid"];
  manifold -> tria        [color="black",style="solid"];
  manifold -> mapping     [color="black",style="solid"];
  node [fontname="FreeSans",fontsize=12,
        shape=record,height=0.2,width=0.4,
        color="gray", fontcolor="gray", fillcolor="white", style="filled"];
  edge [color="gray", weight=1];
  opencascade [label="OpenCASCADE"];
  subgraph misclibs {
  systems    [label="Operators", fillcolor="deepskyblue"];
  }
  opencascade -> manifold [dir="none"];

  node [fontname="FreeSans",fontsize=12,
        shape=ellipse,height=0.2,width=0.4,
        color="gray", fontcolor="gray", fillcolor="white", style="filled"];
  edge [color="gray", weight=1];
  gmsh        [label="gmsh", URL="\ref Gmsh"];
  visit       [label="VisIt"]
  paraview    [label="ParaView"]
  gmsh -> tria       [dir="none"];
  output -> visit    [dir="none"];
  output -> paraview [dir="none"];
}
 * @enddot
 *
 * In essence, the framework provided by the FEEvaluation class on top of the
 * data storage in MatrixFree is a specialized operator evaluation
 * framework. It is currently only compatible with a subset of the elements
 * provided by the library which have a special structure, namely those where
 * the basis can be described as a tensor product of one-dimensional
 * polynomials. This opens for efficient transformation between vector entries
 * and values or gradients in quadrature points with a technique that is
 * called sum factorization. This technique has its origin in the spectral
 * element community, started by the work of Orszag in 1980. While this
 * technique is initially nothing else than a particular technique for
 * assembling vectors (or matrices) that is faster than the general-purpose
 * vehicle FEValues, its efficiency makes it possible to use these integration
 * facilities to directly evaluate the matrix-vector products in iterative
 * solvers, rather than first assembling a matrix and then using that matrix
 * for doing matrix-vector products. This step is initially non-intuitive and
 * goes against what many people were taught in their mathematics and computer
 * science education, including most of the deal.II developers, because it
 * appears to be wasteful to re-compute integrals over and over again, instead
 * of using precomputed data. However, as the tutorial programs step-37,
 * step-48, step-59, step-64, and step-67 show, these concepts usually
 * outperform traditional algorithms on modern computer architectures.
 *
 * The two main reasons that favor matrix-free computations are the
 * following:
 * <ol>
 * <li> Matrix-free methods skip the storage of big global sparse matrices and
 * compute the underlying weak forms on the fly. Since the memory transfer,
 * i.e., the speed at which the data can be read from RAM memory, is the
 * bottleneck for matrix-based computations rather than the actual arithmetic
 * done using this data, a matrix-free evaluation that reads less data can be
 * advantageous even if it does more computations. This concept is building
 * upon a trend in computer architecture which is best described by the term
 * <i>memory wall</i>, saying that compute performance has increased more
 * rapidly than the memory performance. Thus, a certain degree of arithmetic
 * operations is essentially for free, and this share has become larger during
 * the last twenty years. It has enabled this radical algorithm switch going
 * from a matrix-based to a matrix-free implementation of matrix-vector
 * products for iterative solvers, besides their classical use in explicit
 * time integration. Of course, the implementation must be efficient and there
 * cannot be an excess in computations to make it a win in total. The deal.II
 * library uses SIMD vectorization and highly optimized kernels based on
 * templates of the polynomial degree to achieve this goal. To give a
 * perspective, a sparse matrix-vector product for quadratic elements FE_Q
 * used to be equally fast as the matrix-free implementation on processors
 * designed around 2005-2007 (e.g. Pentium 4 or AMD Opteron Barcelona
 * with 2-4 cores per chip). By 2018, the matrix-free evaluation is around
 * eight times as fast (measured on Intel Skylake Server, 14 cores).
 * <li> Matrix-free methods have a better complexity per degree of freedom as
 * the degree is increased, due to sum factorization. The work per degree of
 * freedom increases as $\mathcal O(k)$ in the degree $k$ for matrix-free
 * schemes, whereas it increases as $\mathcal O(k^d)$ for matrix-based
 * methods. This gives higher order schemes an edge. A particularly nice
 * feature in matrix-free evaluation is that the $\mathcal O(1)$ terms often
 * dominate, so it appears that higher order methods are as fast in terms of
 * evaluation time as low order ones, when they have the same number of
 * degrees of freedom. For the implementation in deal.II, best throughput is
 * typically achieved for polynomial degrees between three and six.
 * </ol>
 *
 * To summarize, matrix-free computations are the way to go for higher order
 * elements (where higher order means everything except linear shape
 * functions) and use in explicit time stepping (step-48) or iterative solvers
 * where also preconditioning can be done in a matrix-free way, as
 * demonstrated in the step-37 and step-59 tutorial programs.
 *
 * <h3>The matrix-free evaluation infrastructure</h3>
 *
 * The top level interface is provided by the FEEvaluation class, which also
 * contains an extensive description of different use cases.
 *
 * <h4>The FEEvaluation class hierarchy</h4>
 *
 * The class FEEvaluation is derived from the class FEEvaluationAccess, which
 * in turn inherits from FEEvaluationBase. The FEEvaluation class itself is
 * templated not only on the dimension, the number of components, and the
 * number type (e.g. double or float), but also on the polynomial degree and
 * on the number of quadrature points per spatial direction. This information
 * is used to pass the loop lengths in sum factorization to the respective
 * kernels (see `tensor_product_kernels.h` and
 * `evaluation_kernels.h`) and ensure optimal
 * efficiency. All methods that access the vectors or provide access into the
 * data fields on an individual quadrature point are inherited from
 * FEEvaluationAccess.
 *
 * The motivation for the FEEvaluationAccess classes is to allow for
 * specializations of the value and gradient access of interpolated solution
 * fields depending on the number of components. Whereas the base class
 * FEEvaluationBase returns the gradient as a
 * `Tensor<1,n_components,Tensor<1,dim,VectorizedArray<Number>>>`, with the
 * outer tensor going over the components and the inner tensor going through
 * the `dim` components of the gradient. For a scalar field, i.e.,
 * `n_components=1`, we can skip the outer tensor and simply use
 * `Tensor<1,dim,VectorizedArray<Number>>` as the gradient type. Likewise, for
 * a system with `n_components=dim`, the appropriate format for the gradient
 * is `Tensor<2,dim,VectorizedArray<Number>>`.
 *
 * <h4>The FEFaceEvaluation class</h4>
 *
 * Face integrals, like for inhomogeneous Neumann conditions in continuous FEM
 * or for the large class of discontinuous Galerkin schemes, require the
 * evaluation of quantities on the quadrature point of a face, besides the
 * cell integrals. The facilities for face evaluation are mostly shared with
 * FEEvaluation, in the sense that FEFaceEvaluation also inherits from
 * FEEvaluationAccess. All data fields regarding the degrees of freedom and
 * shape functions can be reused, the latter because all information consists
 * of 1D shape data anyway. With respect to the mapping data, however, a
 * specialization is used because the data is of `structdim=dim-1`. As a
 * consequence, the FEEvaluationAccess and FEEvaluationBase are given a
 * template argument `is_face` to hold pointers to the cell and face mapping
 * information, respectively. Besides access to the function values with
 * FEEvaluationAccess::get_value() or gradients with
 * FEEvaluationAccess::get_gradient(), the face evaluator also enables the
 * access to the normal vector by FEEvaluationAccess::normal_vector() and
 * a specialized field FEEvaluationAccess::get_normal_derivative(), which
 * returns the derivative of the solution field normal to the face. This
 * quantity is computed as the gradient (in real space) multiplied by the
 * normal vector. The combination of the gradient and normal vector is typical
 * of many (simple) second-order elliptic equations, such as the
 * discretization of the Laplacian with the interior penalty method. If the
 * gradient alone is not needed, the combined operation significantly reduces
 * the data access, because only `dim` data entries for `normal * Jacobian`
 * per quadrature point are necessary, as opposed to `dim^2` fields for the
 * Jacobian and `dim` fields for the normal when accessing them individually.
 *
 * An important optimization for the computation of face integrals is to think
 * about the amount of vector data that must be accessed to evaluate the
 * integrals on a face. Think for example of the case of FE_DGQ, i.e.,
 * Lagrange polynomials that have some of their nodes on the element
 * boundary. For evaluation of the function values, only $(k+1)^{d-1}$ degrees
 * of freedom contribute via a non-zero basis function, whereas the rest of
 * the $(k+1)^d$ basis functions evaluate to zero on that boundary. Since
 * vector access is one of the bottlenecks in matrix-free computations, the
 * access to the vector should be restricted to the interesting entries. To
 * enable this setup, the method FEFaceEvaluation::gather_evaluate() (and
 * FEFaceEvaluation::integrate_scatter() for the integration equivalent)
 * combines the vector access with the interpolation to the quadrature
 * points. There exist two specializations, including the aforementioned
 * "non-zero" value case, which is stored as the field
 * internal::MatrixFreeFunctions::ShapeInfo::nodal_at_cell_boundaries. A
 * similar property is also possible for the case where only the value and the
 * first derivative of a selected number of basis functions evaluate to
 * nonzero on a face. The associated element type is FE_DGQHermite and the
 * decision is stored on the property
 * internal::MatrixFreeFunctions::tensor_symmetric_hermite. The
 * decision on whether such an optimized kernel can be used is made
 * automatically inside FEFaceEvaluation::gather_evaluate() and
 * FEFaceEvaluation::integrate_scatter(). It might seem inefficient to do this
 * decision for every integration task, but in the end this is a single `if`
 * statement (conditional jump) that is easily predictable for a modern CPU as
 * the decision is always the same inside an integration loop. (One only pays
 * by somewhat increased compile times because the compiler needs to generate
 * code for all paths, though).
 *
 * <h3>The data storage through the MatrixFree class</h3>
 *
 * The tasks performed by FEEvaluation and FEFaceEvaluation can be split into
 * the three categories: <i>index access into vectors</i>, <i>evaluation and
 * integration on the unit cell</i>, and <i>operation on quadrature points
 * including the geometry evaluation</i>. This split is reflected by the major
 * data fields contained by MatrixFree, using
 * internal::MatrixFreeFunctions::DoFInfo,
 * internal::MatrixFreeFunctions::ShapeInfo, and
 * internal::MatrixFreeFunctions::MappingInfo for each these three categories,
 * respectively. Their design principles and internal layout is described in
 * the following subsections.
 *
 * The main interface all these data structure adhere to is that integration
 * tasks are broken down into a range of cells or faces that one can index
 * into by a single integer index. The information about an integer range for
 * the cell integrals, inner face integrals, and boundary integrals is
 * provided by the class internal::MatrixFreeFunctions::TaskInfo, using the
 * data fields `cell_partition_data`, `face_partition_data`, and
 * `boundary_partition_data`. This class also contains information about
 * subranges of indices for scheduling tasks in parallel using threads, and a
 * grouping of the index range within `{cell,face,boundary}_partition_data`
 * for interleaving cell and face integrals such that the access to vector
 * entries for cell and face integrals re-uses data already in caches.
 *
 * <h4>Index storage: the internal::MatrixFreeFunctions::DoFInfo struct</h4>
 *
 * The main purpose of the DoFInfo class is to provide the indices consumed by
 * the vector access functions FEEvaluationBase::read_dof_values() and
 * FEEvaluationBase::distribute_local_to_global(). The indices are laid out as
 * follows:
 * <ol>
 * <li>Indices are stored in MPI-local index space to enable direct array
 * access, rather than translating a global index into a local one. The latter
 * would be absolutely detrimental to performance.</li>
 * <li>The indices are stored in a field called
 * internal::MatrixFreeFunctions::DoFInfo::dof_indices, which is a long index
 * array. The access granularity in terms of a <i>cell index</i> is controlled
 * by the auxiliary field internal::MatrixFreeFunctions::DoFInfo::row_starts
 * that is similar to the rowstart index in a compressed matrix storage. The
 * scheme supports variable lengths because we support hp-adaptivity and index
 * indirections due to constraints that are contained in the main index
 * array. Due to vectorization over cells, the access granularity would
 * initially be in terms of <i>cell batches</i>. However, we must be able to
 * access also an individual cell, for example for face integrals with the
 * batches of faces that are in general different from the cell batches and
 * access is thus not linear. Furthermore, the support for multi-component
 * systems becomes transparent if we provide a <i>start index</i> to every
 * single component separately. Thus, the `row_starts` field is of length
 * `n_cell_batches()*VectorizedArray<Number>::%size()*n_components`.
 * </li>
 * <li> The translation between components within a system of multiple base
 * elements is controlled by the four variables
 * <ol>
 *   <li>`std::vector<unsigned int> n_components` (components per base element),
 *     </li>
 *   <li>`std::vector<unsigned int> start_components` (translation from the base
 *     element to the unique component number),</li>
 *   <li>`std::vector<unsigned int> component_to_base_index` (translation from
 *     unique component number to base index), and </li>
 *   <li>`std::vector<std::vector<unsigned int>> component_dof_indices_offset`
 *    (offset of the particular component's range of degrees of freedom within
 *    the full list of degrees of freedom on a cell).</li>
 * </ol>
 * </li>
 *
 * <li>Information to extract the FE index in hp-adaptive computations.</li>
 * <li>Information about the 'first access' into a particular vector entry
 * that is used to zero out the entries in a destination vectors within the
 * MatrixFree::loop shortly before accessing them the first time. This is used
 * to avoid writing zeros to the whole vector which destroys data locality.</li>
 * </ol>
 *
 * The setup of the data structures in internal::MatrixFreeFunctions::DoFInfo
 * is done in internal::MatrixFreeFunctions::DoFInfo::read_dof_indices, where we
 * first assume a very general finite element layout, be it continuous or
 * discontinuous elements, and where we resolve the constraints due to hanging
 * nodes. This initial step is done in the original ordering of cells. In a
 * later stage, these cells will in general be rearranged to reflect the order
 * by which we go through the cells in the final loop, and we also look for
 * patterns in the DoF indices that can be utilized, such as contiguous index
 * ranges within a cell. This reordering is done to enable overlap of
 * communication and computation with MPI (if enabled) and to form better group
 * of batches with vectorization over cells. The data storage of indices is
 * linear in this final order, and arranged in
 * internal::MatrixFreeFunctions::DoFInfo::reorder_cells.
 *
 * Since the amount of data to store indices is not negligible, it is
 * worthwhile to reduce the amount of data for special configurations that
 * carry more structure. One example is the case of FE_DGQ where a single
 * index per cell is enough to describe all its degrees of freedom, with the
 * others coming in consecutive order. The class
 * internal::MatrixFreeFunctions::DoFInfo contains a special array of vectors
 * internal::MatrixFreeFunctions::DoFInfo::dof_indices_contiguous that
 * contains a single number per cell. Since both cell and face integrals use
 * different access patterns and the data in this special case is small, we
 * are better off storing 3 such vectors, one for the faces decorated as
 * `interior` (index 0), one for the faces decorated as `exterior` (index 1),
 * and one for the cells (index 2), rather than using the indirection through
 * internal::MatrixFreeFunctions::FaceInfo. There is a series of additional
 * special storage formats available in DoFInfo. We refer to the documentation
 * of the struct internal::MatrixFreeFunctions::DoFInfo::IndexStorageVariants
 * for the options implemented in deal.II and their motivation.
 *
 * Finally, the DoFInfo class also holds a shared pointer describing the
 * parallel partitioning of the vectors. Due to the restriction of
 * Utilities::MPI::Partitioner, the indices within an individual DoFHandler
 * object passed to the MatrixFree::reinit() function must be contiguous
 * within each MPI process, i.e., the local range must consist of at most one
 * chunk. Besides the basic partitioner, the class also provides a set of
 * tighter index sets involving only a subset of all ghost indices that are
 * added to the vectors' ghost range. These exchange patterns are designed to
 * be combined with the reduced index access via the
 * internal::MatrixFreeFunctions::ShapeInfo::nodal_at_cell_boundaries for
 * example.
 *
 * The MatrixFree class supports multiple DoFHandler objects to be passed to
 * the MatrixFree::reinit() function. For each of these DoFHandler objects, a
 * separate internal::MatrixFreeFunctions::DoFInfo object is created. In
 * MatrixFree, we store a `std::vector` of
 * internal::MatrixFreeFunctions::DoFInfo objects to account for this fact.
 *
 * <h4>The internal::MatrixFreeFunctions::ShapeInfo structure</h4>
 *
 * The evaluation of one-dimensional shape functions on one-dimensional
 * quadrature points is stored in the class
 * internal::MatrixFreeFunctions::ShapeInfo. More precisely, we hold all
 * function values, gradients, and hessians. Furthermore, the values and
 * derivatives of shape functions on the faces, i.e., the points 0 and 1 of
 * the unit interval, are also stored. For face integrals on hanging nodes,
 * the coarser of the two adjacent cells must interpolate the values not to
 * the full quadrature but to a subface only (evaluation points either scaled
 * to [0, 1/2] or [1/2, 1]). This case is handled by the data fields
 * `values_within_subface`, `gradients_within_subface`, and
 * `hessians_within_subface`. This data structure also checks for symmetry in
 * the shape functions with respect to the center of the reference cell (in
 * which case the so-called even-odd transformation is applied, further
 * reducing computations).
 *
 * <h4>The internal::MatrixFreeFunctions::MappingInfo structure</h4>
 *
 * The evaluated geometry information is stored in the class
 * internal::MatrixFreeFunctions::MappingInfo. Similarly to the
 * internal::MatrixFreeFunctions::DoFInfo class, multiple variants are
 * possible within a single MatrixFree instance, in this case based on
 * multiple quadrature formulas. Furthermore, separate data for both cells and
 * faces is stored. Since there is more logic involved and there are
 * synergies between the fields, the `std::vector` of fields is kept within
 * internal::MatrixFreeFunctions::MappingInfo. The individual field is of type
 * internal::MatrixFreeFunctions::MappingInfoStorage and holds arrays with the
 * inverse Jacobians, the JxW values, normal vectors, normal vectors times
 * inverse Jacobians (for FEEvaluationAccess::get_normal_derivative()),
 * quadrature points in real space, and quadrature points on the reference
 * element. We use an auxiliary index array that points to the start of the
 * data for each cell, namely the `data_index_offsets` field for the
 * Jacobians, JxW values, and normal vectors, and `quadrature_point_offsets`
 * for the quadrature points. This offset enables hp-adaptivity with variable
 * lengths of fields similar to what is done for DoFInfo, but it also enables
 * something we call <i>geometry compression</i>. In order to reduce the data
 * access, we detect simple geometries of cells where Jacobians are constant
 * within a cell or also across cells, using
 * internal::MatrixFreeFunctions::MappingInfo::cell_type:
 *
 * <ol>
 * <li> Cartesian cells are cells where the Jacobian is diagonal and the same
 * on every quadrature point of the cell. Only a single field needs to be
 * stored per cell. Due to the similarity within the cell, we also check for
 * other cell batches with the same Jacobian for all cells on the current
 * processor. This can further reduce the memory access. Since the JxW values
 * in the general case store the Jacobian times the quadrature weight, but we
 * only want to keep a single field for a Cartesian cell, we misuse the name
 * <i>JxW</i> in the Cartesian case and only store the determinant of the
 * Jacobian, without the quadrature weight. As a consequence, we need to be
 * careful in FEEvaluationBase::submit_value() and similar for this case as we
 * must still multiply by the weight.
 * <li> Affine cells have constant Jacobian within the whole cell, so only a
 * single field needs to be stored per cell. Due to the similarity within the
 * cell, we also check for other cell batches with the same Jacobian for all
 * cells on the current processor. Since the JxW values in the general case
 * store the Jacobian times the quadrature weight, but we only want to keep a
 * single field for an affine cell, we misuse the name <i>JxW</i> in the
 * affine case, just as in the Cartesian case, and only store the determinant
 * of the Jacobian, without the quadrature weight. As a consequence, we need
 * to be careful in FEEvaluationBase::submit_value() and similar for this case
 * as we must still multiply by the weight.
 * <li> On faces, we can have the special case that the normal vector is the
 * same in all quadrature points also when the JxW values are different. This
 * is the case for faces which are flat. To reduce the data access, we keep
 * this as a third option of compressed indices in
 * internal::MatrixFreeFunctions::GeometryType. As opposed
 * to the Cartesian and affine case where only a single field is reserved in
 * the arrays, flat faces keep a separate entry for all quadrature points (to
 * keep a single index field `data_index_offsets`), but only access the first
 * one.
 * <li> The general type indices a cell or face where no compression was
 * found. In this case, we also do not look for opportunities to find the same
 * pattern on more than one cell, even though such cases might exist such as
 * for extruded meshes. This search operation, which is based on inserting
 * data into a `std::map` using a custom floating point comparator
 * `FPArrayComparator`, is efficient enough when a single data field per cell
 * is used. However, it would be pretty expensive if done for all quadrature
 * points of all cells (with many different cases).
 * </ol>
 *
 * The implementation of internal::MatrixFreeFunctions::MappingInfo is split
 * into cell and face parts, so the
 * two components can be easily held apart. What makes the code a bit awkward
 * to read is the fact that we need to batch several objects together from the
 * original scalar evaluation done in an FEValues object, that we need to
 * identify data fields that are repetitive, and that we need to define the
 * compression over several cells with a `std::map` for the Cartesian and
 * affine cases.
 *
 * The data computation part of internal::MatrixFreeFunctions::MappingInfo is
 * parallelized by tasks besides
 * the obvious MPI parallelization. Each processor computes the information
 * on a subrange, before the data is eventually copied into a single combined
 * data field.
 *
 * <h3>Identification and parallelization of face integrals</h3>
 *
 * The current scheme for face integrals in MatrixFree builds an independent
 * list of tasks for all of the faces, rather than going through the `2*dim`
 * faces of a cell explicitly. This has the advantage that all information on
 * a face is processed only once. Typical DG methods compute numerical fluxes
 * that are conservative, i.e., that look the same from both sides of the face
 * and whatever information leaves one cell must exactly enter the neighbor
 * again. With this scheme, they must only be computed once. Also, this
 * ensures that the geometry information must only be loaded once, too. (A
 * possible disadvantage is that a face-based approach with independent
 * numbering makes thread-based parallelism much more complicated than a
 * cell-based approach where only the information of the current cell is
 * written into and neighbors are only read.)
 *
 * Since faces are independent of cells, they get their own layout of
 * vectorization. It is the nature of faces that whatever is a contiguous
 * batch of cells gets intertwined when seen from a batch of faces (where we
 * only keep faces together that have the same face index within a cell and so
 * on). The setup of the face loop, which is done in the file
 * `face_setup_internal.h`, tries to provide face
 * batches that at least partly resemble the cell patches, to increase the
 * data locality. Along these lines, the face work is also interleaved with
 * cell work in the typical MatrixFree::loop context, i.e., the `cell_range`
 * and `face_range` arguments returned to the function calls are usually
 * pretty short.
 *
 * Since all integrals from both sides are performed at once, the question
 * arises which one of the two processors at subdomain boundaries is assigned
 * a face. The authors of this module have performed extensive experiments and
 * found out that the scheme that is applied for the degree of freedom
 * storage, namely to assign all items with possible overlap to a single
 * processor, is pretty imbalanced with up to 20% difference in the number of
 * faces. For better performance, a balanced scheme is implemented in
 * `face_setup_internal.h` that splits all interfaces between each pair of
 * processors into two chunks, one being done by one processor and one by the
 * other. Even though this increases the number of messages to be sent over
 * MPI, this is worth it because the load gets more balanced. Also, messages
 * are rather big at around 5-50kB when the local problem size is 100,000 DoFs
 * in 3D. At this message size, the latency is typically less than the
 * throughput anyway.
 *
 * Face data is not initialized by default, but must be triggered by the face
 * update flags in MatrixFree::AdditionalData, namely
 * `mapping_update_flags_inner_faces` or `mapping_update_flags_boundary_faces`
 * set to a value different from `update_default`.
 *
 * <h3>Invoking MatrixFree::loop</h3>
 *
 * The MatrixFree class supports two types of loops over the entities. The
 * first one, which has been available on the deal.II master branch since
 * 2012, is to only perform cell integrals, using one of the three `cell_loop`
 * functions that takes a function pointer to the cell operation. The second
 * setup, introduced in 2018, is a loop where also face and/or boundary
 * integrals can be performed, called simply `loop`. This takes three function
 * pointers, addressing the cell work, inner face work, and boundary face
 * work, respectively.
 *
 * Besides scheduling the work in an appropriate way, the loop performs two
 * more tasks:
 * <ol>
 * <li> Data exchange on the `src` and `dst` vector, calling
 * `update_ghost_values()` and `compress(VectorOperation::add)`,
 * respectively. The exchange can be done in an asynchronous fashion
 * overlapping the communication with work on cells that do not need data from
 * remote processors, if the respective flag
 * MatrixFree::AdditionalData::overlap_communication_computation is set to
 * true (the default).
 * <li> Zero the `dst` vector using the respective flag. The advantage of
 * doing this inside the loop is that the loop knows which entries in the
 * vectors are (first) touched by some of the subranges in the cell and face
 * loops. Thus, it can zero the vector piece by piece to ensure that we do not
 * need to access the vector entries twice (once for zeroing, once for adding
 * contributions). This might seem like a tiny optimization, but indeed the
 * operator evaluation can be so quick that simply zeroing a vector can take
 * around 20% of the operator evaluation time, so it is really worth the
 * effort! Since there is some experimentation to this parameter, the DoFInfo
 * class keeps a static variable
 * internal::MatrixFreeFunctions::DoFInfo::chunk_size_zero_vector where this
 * can be adjusted (if someone thinks that something else would be better, for
 * example because future computers look different than they did in 2018 when
 * this was introduced).
 * </ol>
 *
 * Finally, the MatrixFree::loop functions also take an argument to pass the
 * type of data access on face integrals, described by the struct
 * MatrixFree::DataAccessOnFaces, to reduce the amount of data that needs to
 * be exchanged between processors. Unfortunately, there is currently no way
 * of communicating this information, that gets available inside
 * FEFaceEvaluation by the combination of the type of evaluation (values
 * and/or gradients) and the underlying shape functions, to the
 * MatrixFree::loop for avoiding to manually set this kind of information at a
 * second spot.
 *
 * <h3>Representation of constraints with matrix-free operators</h3>
 *
 * The MatrixFree object creates an efficient internal representation of
 * constraints, in order to more efficiently deal with resolving the
 * constraints while the entries of solution vectors are read on each cell
 * (done by FEEvaluation::read_dof_values()). This means that
 * AffineConstraints::distribute() is not necessary to be called on an input
 * vector to a matrix-free loop to get a local solution representation that is
 * compatible with the constraints. Likewise, the contribution of integrals
 * from constrained to unconstrained entries is done within
 * FEEvaluation::distribute_local_to_global() or
 * FEEvaluation::integrate_scatter(), replacing the slower function
 * AffineConstraints::distribute_local_to_global(). There is one downside with
 * this integrated approach, namely when considering larger-scale applications
 * with different kinds of constraints, in particular inhomogeneous
 * constraints that might possibly change from one time-step (with an
 * associated solve of a linear system) to the next.
 *
 * At this point, it is useful to clarify the concept of a homogeneous
 * operator, which is the basic case discussed in the step-37 tutorial program
 * with homogeneous Dirichlet boundary conditions, and where a matrix-free
 * operator can be easily set up. In the form discussed there, the solution
 * $u_h$ is represented by a linear combination of the basis functions using
 * only the unknown coefficients $U_j$. A more general finite element problem
 * might involve non-zero boundary values, which include an affine
 * contribution in the solution $u_h$ and represented by inhomogeneous
 * constraints. For a more detailed explanation of constraints, see also the
 * introduction of AffineConstraints.  The affine contribution ends up at the
 * right-hand side of the linear system, whereas the actually unknown
 * coefficients are determined by solving a linear system. Let us discuss this
 * case for both continuous finite elements, where those inhomogeneous
 * boundary values are typically imposed strongly, and for discontinuous
 * elements with a weak imposition of data separately in the next two
 * subsections.
 *
 * <h4>Representation of inhomogeneous Dirichlet boundary conditions for
 * continuous finite elements</h4>
 *
 * In analogy to the case of setting up a classical matrix-based linear
 * system, the contributions of inhomogeneous data need to be split off from
 * the part involving the unknown solution coefficients. For explaining how to
 * split the contributions, let us start by looking at a solution
 * representation of the form
 * @f{eqnarray*}{
 * u_h(\mathbf{x}) = \sum_{j\in \mathcal N} \varphi_i(\mathbf{x}) U_j =
 * \sum_{j\in \mathcal N \setminus \mathcal N_D} \varphi_j(\mathbf{x}) U_j +
 * \sum_{j\in \mathcal N_D} \varphi_j(\mathbf{x}) g_j.
 * @f}
 * Here, $U_j$ denotes the nodal values of the solution and $\mathcal N$
 * denotes the set of all nodes. The set $\mathcal N_D\subset \mathcal N$ is
 * the subset of the nodes that are subject to Dirichlet boundary conditions
 * where the solution is forced to equal $U_j = g_j = g(\mathbf{x}_j)$ as the
 * interpolation of boundary values on the Dirichlet-constrained node points
 * $j\in \mathcal N_D$.
 *
 * This form of the solution can be inserted into a general PDE system with
 * bilinear form $b(\varphi, u)$ and linear form $f(\varphi)$ (for the
 * Laplacian considered in step-37, we have $b(\varphi, u) = (\nabla \varphi,
 * \nabla u)_\Omega$ and $f(\varphi) = (\varphi, f)_\Omega$), which needs to
 * be fulfilled for all basis function $\varphi$ in some discrete function
 * space. We separate the unknown coefficients $U_j$ from known quantities,
 * which are moved to the right-hand side. A finite element problem seeks for
 * the coefficients $(U_j)_{j \in \mathcal N \setminus \mathcal N_D}$ such
 * that there holds for all test functions $\varphi_i,\ i \in \mathcal N
 * \setminus \mathcal N_D$
 * @f{eqnarray*}{
 * b(\varphi_i, u_h) &=& f(\varphi_i) \quad \Rightarrow \                \
 * \sum_{j\in \mathcal N \setminus \mathcal N_D}b(\varphi_i, \varphi_j) \, U_j &=&
 * f(\varphi_i) -\sum_{j\in \mathcal N_D} b(\varphi_i,\varphi_j)\, g_j.
 * @f}
 *
 * For matrix-based methods, the contributions resulting from inhomogeneous
 * constraints are inserted into the right-hand side vector by the function
 * AffineConstraints::distribute_local_to_global(). A similar operation is
 * performed by MatrixTools::apply_boundary_values(). More precisely, the
 * contribution from the cell matrix associated with unknown coefficients
 * $U_j$ end up in the matrix, whereas the information from an inhomogeneous
 * constraint $g_j$ is subtracted from the entry $i$ of the vector, multiplied
 * by the local $(i,j)$ entry of the cell matrix as the weight.  However, a
 * matrix-free operator cannot make use of this infrastructure. This is the
 * result of the main architecture of matrix-free methods, which evaluate the
 * operator action, i.e., the left-hand side represented by the weak form
 * above, within facilities like iterative solvers, where the contribution
 * ending up in a possible right-hand side is ignored. Recall that the
 * right-hand side is set up in a different stage of the solution
 * algorithm. It is therefore crucial to think of the operator evaluation in
 * terms of the solution as two parts, a part representing the homogeneous PDE
 * operator (subject to zero Dirichlet conditions) and a part that represents
 * the inhomogeneity.
 *
 * As a result, evaluators defined by MatrixFree::cell_loop() or
 * MatrixFree::loop() represent the matrix-vector product of a
 * <b>homogeneous</b> operator (the left-hand side of the last formula). In
 * this setup, it does not matter whether the AffineConstraints object passed
 * to the MatrixFree::reinit() contains inhomogeneous constraints or not, the
 * MatrixFree::cell_loop() call will only resolve the homogeneous part of the
 * constraints as long as it represents a <b>linear</b> operator.
 *
 * For the right-hand side computation, where the contribution of
 * inhomogeneous conditions ends up, different evaluation functions compared
 * to the above setting are necessary, and we need to explicitly generate the
 * data that enters the right-hand side rather than using a byproduct of the
 * matrix assembly. The generic approach is to evaluate the differential
 * operator using the terms involving $g_j$ in the equation above, using a
 * vector that only contains the Dirichlet values and zeros elsewhere:
 *
 * @code
 * // interpolate boundary values on vector solution
 * std::map<types::global_dof_index, double> boundary_values;
 * VectorTools::interpolate_boundary_values(mapping,
 *                                          dof_handler,
 *                                          map_of_boundary_ids_and_functions,
 *                                          boundary_values);
 * for (const auto [boundary_dof_index, boundary_value] : boundary_values)
 *   if (solution.locally_owned_elements().is_element(boundary_dof_index))
 *     solution(boundary_dof_index) = boundary_value;
 * @endcode
 * or, equivalently, if the inhomogeneous constraints are contained in
 * an AffineConstraints object,
 * @code
 * solution = 0;
 * inhomogeneous_constraints.distribute(solution);
 *@endcode
 *
 * Note, however, that this step only sets entries in constrained DoFs, which
 * would be ignored by FEEvaluation::read_dof_values() that gets used to
 * define the terms in $\sum_{j\in \mathcal N_D} b(\varphi_i,\varphi_j)\, g_j$
 * with the chosen vector `solution`. There are therefore two general options
 * to solve this situation:
 *
 * <h5> Using FEEvaluation::read_dof_values_plain() that does not resolve
 * constraints </h5>
 *
 * To account for the case where constrained degrees of freedom contain data
 * that is generated from inhomogeneous data, the built-in resolution of
 * constraints for a homogeneous operator of FEEvaluation::read_dof_values()
 * can be by-passed, using the function FEEvaluation::read_dof_values_plain()
 * instead. An example code could look as follows:
 * @code
 * {
 *   solution = 0;
 *   inhomogeneous_constraints.distribute(solution);
 *   solution.update_ghost_values();
 *   system_rhs = 0;
 *
 *   const Table<2, VectorizedArray<double>> &coefficient = system_matrix.get_coefficient();
 *   FEEvaluation<dim, degree_finite_element> phi(*system_matrix.get_matrix_free());
 *   for (unsigned int cell = 0;
 *        cell < system_matrix.get_matrix_free()->n_cell_batches();
 *        ++cell)
 *     {
 *       phi.reinit(cell);
 *       phi.read_dof_values_plain(solution);
 *
 *       // Implement the underlying differential equation, using the same
 *       // of phi.evaluate(...), operation at quadrature points and
 *       // phi.integrate(...) as in the evaluation of the matrix-vector
 *       // product, but use a negative sign for the contribution.
 *       // Furthermore, also add the terms to be added to the
 *       // right-hand side of the PDE problem here.
 *
 *       phi.distribute_local_to_global(system_rhs);
 *     }
 *   system_rhs.compress(VectorOperation::add);
 * }
 * @endcode
 *
 * Notice the use of FEEvaluation::read_dof_values_plain() in this function,
 * which ignores all constraints. Due to this setup, the code must make sure
 * that other (homogeneous) constraints, e.g. by hanging nodes or periodicity,
 * are correctly distributed to the input vector already because none of the
 * constraints is considered in FEEvaluation::read_dof_values_plain().
 *
 * Note that the negative sign for the terms containing the $g_j$ terms
 * (representing a subtraction of boundary data) according to the formula at
 * the beginning of this subsection is following a more general concept,
 * namely Newton's method for nonlinear equations applied to a linear
 * system. As an initial guess for the variable @p solution, the inhomogeneous
 * Dirichlet boundary conditions has been used, leading to the residual $r = f
 * - Au_0$. The linear system would then be solved as $\Delta u = A^{-1}
 * (f-Au)$, and we would conclude the step by computing the actual solution $u
 * = u_0 + \Delta u$. For a linear system, we reach the exact solution after a
 * single iteration. For a general non-linear system, the code described above
 * would be part of a @p assemble_residual() function, where the residual is
 * computed for a complete representation of a solution field (with initially
 * un-converged state but correct value for the inhomogeneously constrained
 * unknowns), which gets updated increments with zero boundary data, i.e., a
 * homogeneous operator.
 *
 * This scenario is explained in the step-50 and step-66 tutorial programs.
 *
 * <h5> Using a second AffineConstraints object without Dirichlet conditions </h5>
 *
 * It is also possible to avoid going through AffineConstraints::distribute()
 * to interpolate all constraints, which might be desirable for high-order
 * finite elements on meshes with many hanging nodes with fast solvers where
 * the generic algorithms of the AffineConstraints::distribute() function can
 * be expensive. This works by equipping MatrixFree::reinit() with a second
 * DoFHandler / AffineConstraints pair (using the same DoFHandler as for the
 * original homogeneous operator), where the second AffineConstraints object
 * only contains homogeneous constraints (such as hanging nodes or periodicity
 * or Dirichlet boundaries that are always zero). This can be set up as
 * follows (for the example of a single DoFHandler; it is of course also
 * possible to add additional DoFHandler objects to create even more
 * complicated scenarios):
 *
 * @code
 * template <int dim>
 * void ApplicationProblem<dim>::setup_matrix_free()
 * {
 *   typename MatrixFree<dim, NumberType>::AdditionalData additional_data;
 *   additional_data.mapping_update_flags =
 *     (update_gradients | update_JxW_values | update_quadrature_points);
 *   matrix_free.reinit(mapping,
 *                      {{&dof_handler, &dof_handler}},
 *                      std::vector<const AffineConstraints<NumberType> *>
 *                        {{&all_constraints, &constraints_homogeneous}},
 *                      QGauss<1>(fe.degree + 1),
 *                      additional_data);
 * }
 *   inhomogeneous_operator.initialize(matrix_free);
 * @endcode
 *
 * In this code, we assume that `all_constraints` contains all constraints to
 * be resolved for the homogeneous operator (which would also be used when
 * assembling a matrix/right-hand side system with
 * AffineConstraints::distribute_local_to_global()), whereas
 * `constraints_homogeneous` refers to only those constraints that are known
 * to be always homogeneous, e.g. hanging nodes and periodicity. In setting up
 * those constraints, we would of course share the construction to make sure
 * the two objects are in sync for the common constraints of both objects.  In
 * the two local evaluators for the residual computation (picking up
 * inhomogeneous constraints) and the actual matrix-vector product, we would
 * then select the following evaluators:
 *
 * @code
 * template <int dim>
 * void
 * ApplicationProblem<dim>::local_residual(
 *   const MatrixFree<dim, NumberType>           &matrix_free,
 *   VectorType                                  &residual,
 *   const VectorType                            &solution,
 *   const std::pair<unsigned int, unsigned int> &range)
 * {
 *   FEEvaluation<dim, fe_degree> eval_inhomogeneous(matrix_free, 1);
 *   FEEvaluation<dim, fe_degree> eval_homogeneous(matrix_free, 0);
 *
 *   for (unsigned int cell = range.first; cell < range.second; ++cell)
 *     {
 *       eval_inhomogeneous.reinit(cell);
 *       eval_inhomogeneous.read_dof_values(solution);
 *       eval_inhomogeneous.evaluate(EvaluationFlags::values |
 *                             EvaluationFlags::gradients);
 *       eval_homogeneous.reinit(cell);
 *       for (const auto q : eval_homogeneous.quadrature_point_indices())
 *         {
 *           const auto val  = eval_inhomogeneous.get_value(q);
 *           const auto grad = eval_inhomogeneous.get_gradient(q);
 *           // evaluate PDE and possible forcing terms
 *           eval_homogeneous.submit_value(..., q);
 *           eval_homogeneous.submit_gradient(..., q);
 *         }
 *       eval_homogeneous.integrate(EvaluationFlags::values |
 *                                    EvaluationFlags::gradients);
 *       eval_homogeneous.distribute_local_to_global(residual);
 *     }
 * }
 * @endcode
 *
 * Notice how we set up the inhomogeneous evaluator (associated with reading
 * the solution field including some constraints) with the index '1',
 * corresponding to the second dof_handler / constraints pair we passed to the
 * `setup_matrix_free()` function above, and use that to read the full
 * underlying field. At the same time, we consider a separate evaluator,
 * initialized with index '0', for the homogeneous part (corresponding to test
 * functions) that gets used during the integration part. This makes sure the
 * residual is posed for the system that includes the constraints from the
 * boundary in a correct way.
 *
 * For completeness, the evaluation of the homogeneous operator (matrix-vector
 * product) would operate solely with the first constraints object of index
 * '0'.
 * @code
 * template <int dim>
 * void
 * ApplicationProblem<dim>::local_matrix_vector_product(
 *   const MatrixFree<dim, NumberType>           &matrix_free,
 *   VectorType                                  &dst,
 *   const VectorType                            &src,
 *   const std::pair<unsigned int, unsigned int> &range)
 * {
 *   FEEvaluation<dim, fe_degree> eval_homogeneous(matrix_free, 0);
 *
 *   for (unsigned int cell = range.first; cell < range.second; ++cell)
 *     {
 *       eval_homogeneous.reinit(cell);
 *       eval_homogeneous.read_dof_values(solution);
 *       eval_homogeneous.evaluate(EvaluationFlags::values |
 *                           EvaluationFlags::gradients);
 *       for (const auto q : eval_homogeneous.quadrature_point_indices())
 *         {
 *           const auto val  = eval_homogeneous.get_value(q);
 *           const auto grad = eval_homogeneous.get_gradient(q);
 *           // evaluate PDE with homogeneousogeneous terms
 *           eval_homogeneous.submit_value(..., q);
 *           eval_homogeneous.submit_gradient(..., q);
 *         }
 *       eval_homogeneous.integrate(EvaluationFlags::values |
 *                                    EvaluationFlags::gradients);
 *       eval_homogeneous.distribute_local_to_global(residual);
 *     }
 * }
 * @endcode
 *
 * <h4>Representation of inhomogeneous Dirichlet boundary conditions for
 * discontinuous elements</h4>
 *
 * For discontinuous elements, boundary conditions are typically imposed
 * weakly. This means that contributions from a given field at the boundary
 * need to be evaluated at the location of quadrature points explicitly within
 * the integrals. As a result, the handling with matrix-free operators is
 * similar to the case with matrices, as calculations are needed in both
 * cases. The step-59 tutorial program contains the main concepts. Those can
 * be summarized as follows:
 * <ol>
 * <li>We set up an operator that only evaluates the homogeneous part of
 * the underlying PDE problem for the use as linear operator (matrix) within
 * an iterative solver.</li>
 * <li>We set up an additional operator to only evaluate inhomogeneous parts
 * in the PDE. This can include both the source term of a linear PDE and
 * inhomogeneous Dirichlet or Neumann data.</li>
 * <li>For a non-linear problem, the previous function is replaced by one
 * that evaluates the full operator, including both the solution interpolation
 * from the current linearization point and the inhomogeneous data, alongside
 * with the homogeneous part from the first bullet point.</li>
 * <li>Since no data is imposed via constraints here, the same FEEvaluation
 * object can be used (i.e., from the same DoFHandler/AffineConstraints pair),
 * but with different interpretations and terms in the two cases. It
 * depends on the chosen discretization as to how the two cases look like
 * specifically.</li>
 * </ol>
 *
 * We note that explicit time integrators, such as the one used in the
 * step-67, step-76 and step-89 tutorial programs, use the full operator as
 * described in the third bullet point of the list.
 */
