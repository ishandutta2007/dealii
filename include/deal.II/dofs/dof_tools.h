// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 1999 - 2025 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------

#ifndef dealii_dof_tools_h
#define dealii_dof_tools_h


#include <deal.II/base/config.h>

#include <deal.II/base/exceptions.h>
#include <deal.II/base/index_set.h>
#include <deal.II/base/point.h>

#include <deal.II/dofs/dof_handler.h>

#include <deal.II/fe/component_mask.h>

#include <deal.II/lac/affine_constraints.h>
#include <deal.II/lac/sparsity_pattern_base.h>

#include <map>
#include <ostream>
#include <set>
#include <vector>


DEAL_II_NAMESPACE_OPEN

// Forward declarations
#ifndef DOXYGEN
class BlockMask;
template <int dim, typename RangeNumberType>
class Function;
template <int dim, int spacedim>
class FiniteElement;
namespace hp
{
  template <int dim, int spacedim>
  class MappingCollection;
  template <int dim, int spacedim>
  class FECollection;
} // namespace hp
template <typename MeshType>
DEAL_II_CXX20_REQUIRES(concepts::is_triangulation_or_dof_handler<MeshType>)
class InterGridMap;
template <int dim, int spacedim>
class Mapping;
template <int dim, class T>
class Table;
template <typename Number>
class Vector;

namespace GridTools
{
  template <typename CellIterator>
  struct PeriodicFacePair;
}

namespace DoFTools
{
  namespace internal
  {
    /*
     * Default value of the face_has_flux_coupling parameter of
     * make_flux_sparsity_pattern. Defined here (instead of using a default
     * lambda in the parameter list) to avoid a bug in gcc where the same lambda
     * gets defined multiple times.
     */
    template <int dim, int spacedim>
    inline bool
    always_couple_on_faces(
      const typename DoFHandler<dim, spacedim>::active_cell_iterator &,
      const unsigned int)
    {
      return true;
    }
  } // namespace internal
} // namespace DoFTools

#endif

/**
 * This is a collection of functions operating on, and manipulating the numbers
 * of degrees of freedom. The documentation of the member functions will provide
 * more information, but for functions that exist in multiple versions, there
 * are sections in this global documentation stating some commonalities.
 *
 * <h3>Setting up sparsity patterns</h3>
 *
 * When assembling system matrices, the entries are usually of the form $a_{ij}
 * = a(\phi_i, \phi_j)$, where $a$ is a bilinear functional, often an integral.
 * When using sparse matrices, we therefore only need to reserve space for those
 * $a_{ij}$ only, which are nonzero, which is the same as to say that the basis
 * functions $\phi_i$ and $\phi_j$ have a nonempty intersection of their
 * support. Since the support of basis functions is bound only on cells on which
 * they are located or to which they are adjacent, to determine the sparsity
 * pattern it is sufficient to loop over all cells and connect all basis
 * functions on each cell with all other basis functions on that cell.  There
 * may be finite elements for which not all basis functions on a cell connect
 * with each other, but no use of this case is made since no examples where this
 * occurs are known to the author.
 *
 *
 * <h3>DoF numberings on boundaries</h3>
 *
 * When projecting the traces of functions to the boundary or parts thereof, one
 * needs to build matrices and vectors that act only on those degrees of freedom
 * that are located on the boundary, rather than on all degrees of freedom. One
 * could do that by simply building matrices in which the entries for all
 * interior DoFs are zero, but such matrices are always very rank deficient and
 * not very practical to work with.
 *
 * What is needed instead in this case is a numbering of the boundary degrees of
 * freedom, i.e. we should enumerate all the degrees of freedom that are sitting
 * on the boundary, and exclude all other (interior) degrees of freedom. The
 * map_dof_to_boundary_indices() function does exactly this: it provides a
 * vector with as many entries as there are degrees of freedom on the whole
 * domain, with each entry being the number in the numbering of the boundary or
 * numbers::invalid_dof_index if the dof is not on the boundary.
 *
 * With this vector, one can get, for any given degree of freedom, a unique
 * number among those DoFs that sit on the boundary; or, if your DoF was
 * interior to the domain, the result would be numbers::invalid_dof_index. We
 * need this mapping, for example, to build the
 * @ref GlossMassMatrix "mass matrix"
 * on the boundary (for this, see make_boundary_sparsity_pattern()
 * function, the corresponding section below, as well as the MatrixCreator
 * namespace documentation).
 *
 * Actually, there are two map_dof_to_boundary_indices() functions, one
 * producing a numbering for all boundary degrees of freedom and one producing a
 * numbering for only parts of the boundary, namely those parts for which the
 * boundary indicator is listed in a set of indicators given to the function.
 * The latter case is needed if, for example, we would only want to project the
 * boundary values for the Dirichlet part of the boundary. You then give the
 * function a list of boundary indicators referring to Dirichlet parts on which
 * the projection is to be performed. The parts of the boundary on which you
 * want to project need not be contiguous; however, it is not guaranteed that
 * the indices of each of the boundary parts are continuous, i.e. the indices of
 * degrees of freedom on different parts may be intermixed.
 *
 * Degrees of freedom on the boundary but not on one of the specified boundary
 * parts are given the index numbers::invalid_dof_index, as if they were in the
 * interior. If no boundary indicator was given or if no face of a cell has a
 * boundary indicator contained in the given list, the vector of new indices
 * consists solely of numbers::invalid_dof_index.
 *
 * (As a side note, for corner cases: The question what a degree of freedom on
 * the boundary is, is not so easy.  It should really be a degree of freedom of
 * which the respective basis function has nonzero values on the boundary. At
 * least for Lagrange elements this definition is equal to the statement that
 * the off-point, or what deal.II calls support_point, of the shape function,
 * i.e. the point where the function assumes its nominal value (for Lagrange
 * elements this is the point where it has the function value 1), is located on
 * the boundary. We do not check this directly, the criterion is rather defined
 * through the information the finite element class gives: the FiniteElement
 * class defines the numbers of basis functions per vertex, per line, and so on
 * and the basis functions are numbered after this information; a basis function
 * is to be considered to be on the face of a cell (and thus on the boundary if
 * the cell is at the boundary) according to it belonging to a vertex, line, etc
 * but not to the interior of the cell. The finite element uses the same
 * cell-wise numbering so that we can say that if a degree of freedom was
 * numbered as one of the dofs on lines, we assume that it is located on the
 * line. Where the off-point actually is, is a secret of the finite element
 * (well, you can ask it, but we don't do it here) and not relevant in this
 * context.)
 *
 *
 * <h3>Setting up sparsity patterns for boundary matrices</h3>
 *
 * In some cases, one wants to only work with DoFs that sit on the boundary. One
 * application is, for example, if rather than interpolating non- homogeneous
 * boundary values, one would like to project them. For this, we need two
 * things: a way to identify nodes that are located on (parts of) the boundary,
 * and a way to build matrices out of only degrees of freedom that are on the
 * boundary (i.e. much smaller matrices, in which we do not even build the large
 * zero block that stems from the fact that most degrees of freedom have no
 * support on the boundary of the domain). The first of these tasks is done by
 * the map_dof_to_boundary_indices() function (described above).
 *
 * The second part requires us first to build a sparsity pattern for the
 * couplings between boundary nodes, and then to actually build the components
 * of this matrix. While actually computing the entries of these small boundary
 * matrices is discussed in the MatrixCreator namespace, the creation of the
 * sparsity pattern is done by the create_boundary_sparsity_pattern() function.
 * For its work, it needs to have a numbering of all those degrees of freedom
 * that are on those parts of the boundary that we are interested in. You can
 * get this from the map_dof_to_boundary_indices() function. It then builds the
 * sparsity pattern corresponding to integrals like $\int_\Gamma
 * \varphi_{b2d(i)} \varphi_{b2d(j)} dx$, where $i$ and $j$ are indices into the
 * matrix, and $b2d(i)$ is the global DoF number of a degree of freedom sitting
 * on a boundary (i.e., $b2d$ is the inverse of the mapping returned by
 * map_dof_to_boundary_indices() function).
 *
 * <h3>DoF coupling between surface triangulations and bulk triangulations</h3>
 *
 * When working with Triangulation and DoFHandler objects of different
 * co-dimension, such as a `Triangulation<2,3>`, describing (part of) the
 * boundary of a `Triangulation<3>`, and their corresponding DoFHandler objects,
 * one often needs to build a one-to-one matching between the degrees of freedom
 * that live on the surface Triangulation and those that live on the boundary of
 * the bulk Triangulation. The GridGenerator::extract_boundary_mesh() function
 * returns a mapping of surface cell iterators to face iterators, that can be
 * used by the function map_boundary_to_bulk_dof_iterators() to construct a map
 * between cell iterators of the surface DoFHandler, and the corresponding pair
 * of cell iterator and face index of the bulk DoFHandler. Such map can be used
 * to initialize FEValues and FEFaceValues for the corresponding DoFHandler
 * objects. Notice that one must still ensure that the ordering of the
 * quadrature points coincide in the two objects, in order to build a coupling
 * matrix between the two sytesm.
 *
 * @ingroup dofs
 */
namespace DoFTools
{
  /**
   * The flags used in tables by certain <tt>make_*_pattern</tt> functions to
   * describe whether two components of the solution couple in the bilinear
   * forms corresponding to cell or face terms. An example of using these
   * flags is shown in the introduction of step-46.
   *
   * In the descriptions of the individual elements below, remember that these
   * flags are used as elements of tables of size FiniteElement::n_components
   * times FiniteElement::n_components where each element indicates whether
   * two components do or do not couple.
   */
  enum Coupling
  {
    /**
     * Two components do not couple.
     */
    none,
    /**
     * Two components do couple.
     */
    always,
    /**
     * Two components couple only if their shape functions are both nonzero on
     * a given face. This flag is only used when computing integrals over
     * faces of cells, e.g., in DoFTools::make_flux_sparsity_pattern().
     * Use Coupling::always in general cases where gradients etc. occur on face
     * integrals.
     */
    nonzero
  };

  /**
   * @name DoF couplings
   * @{
   */

  /**
   * Map a coupling table from the user friendly organization by components to
   * the organization by blocks.
   *
   * The return vector will be initialized to the correct length inside this
   * function.
   */
  template <int dim, int spacedim>
  void
  convert_couplings_to_blocks(const DoFHandler<dim, spacedim> &dof_handler,
                              const Table<2, Coupling> &table_by_component,
                              std::vector<Table<2, Coupling>> &tables_by_block);

  /**
   * Given a finite element and a table how the vector components of it couple
   * with each other, compute and return a table that describes how the
   * individual shape functions couple with each other.
   */
  template <int dim, int spacedim>
  Table<2, Coupling>
  dof_couplings_from_component_couplings(
    const FiniteElement<dim, spacedim> &fe,
    const Table<2, Coupling>           &component_couplings);

  /**
   * Same function as above for a collection of finite elements, returning a
   * collection of tables.
   *
   * The function currently treats DoFTools::Couplings::nonzero the same as
   * DoFTools::Couplings::always .
   */
  template <int dim, int spacedim>
  std::vector<Table<2, Coupling>>
  dof_couplings_from_component_couplings(
    const hp::FECollection<dim, spacedim> &fe,
    const Table<2, Coupling>              &component_couplings);
  /** @} */

  /**
   * @name Sparsity pattern generation
   * @{
   */

  /**
   * Compute which entries of a matrix built on the given @p dof_handler may
   * possibly be nonzero, and create a sparsity pattern object that represents
   * these nonzero locations.
   *
   * This function computes the possible positions of non-zero entries in the
   * global system matrix by <i>simulating</i> which entries one would write
   * to during the actual assembly of a matrix. For this, the function assumes
   * that each finite element basis function is non-zero on a cell only if its
   * degree of freedom is associated with the interior, a face, an edge or a
   * vertex of this cell.  As a result, a matrix entry $A_{ij}$ that is
   * computed from two basis functions $\varphi_i$ and $\varphi_j$ with
   * (global) indices $i$ and $j$ (for example, using a bilinear form
   * $A_{ij}=a(\varphi_i,\varphi_j)$) can be non-zero only if these shape
   * functions correspond to degrees of freedom that are defined on at least
   * one common cell. Therefore, this function just loops over all cells,
   * figures out the global indices of all degrees of freedom, and presumes
   * that all matrix entries that couple any of these indices will result in a
   * nonzero matrix entry. These will then be added to the sparsity pattern.
   * As this process of generating the sparsity pattern does not take into
   * account the equation to be solved later on, the resulting sparsity
   * pattern is symmetric.
   *
   * This algorithm makes no distinction between shape functions on each cell,
   * i.e., it simply couples all degrees of freedom on a cell with all other
   * degrees of freedom on a cell. This is often the case, and always a safe
   * assumption. However, if you know something about the structure of your
   * operator and that it does not couple certain shape functions with certain
   * test functions, then you can get a sparser sparsity pattern by calling a
   * variant of the current function described below that allows to specify
   * which vector components couple with which other vector components.
   *
   * The method described above lives on the assumption that coupling between
   * degrees of freedom only happens if shape functions overlap on at least
   * one cell. This is the case with most usual finite element formulations
   * involving conforming elements. However, for formulations such as the
   * Discontinuous Galerkin finite element method, the bilinear form contains
   * terms on interfaces between cells that couple shape functions that live
   * on one cell with shape functions that live on a neighboring cell. The
   * current function would not see these couplings, and would consequently
   * not allocate entries in the sparsity pattern. You would then get into
   * trouble during matrix assembly because you try to write into matrix
   * entries for which no space has been allocated in the sparsity pattern.
   * This can be avoided by calling the DoFTools::make_flux_sparsity_pattern()
   * function instead, which takes into account coupling between degrees of
   * freedom on neighboring cells.
   *
   * There are other situations where bilinear forms contain non-local terms,
   * for example in treating integral equations. These require different
   * methods for building the sparsity patterns that depend on the exact
   * formulation of the problem. You will have to do this yourself then.
   *
   * @param[in] dof_handler The DoFHandler object that describes which degrees
   * of freedom live on which cells.
   *
   * @param[out] sparsity_pattern The sparsity pattern to be filled with
   * entries.
   *
   * @param[in] constraints The process for generating entries described above
   * is purely local to each cell. Consequently, the sparsity pattern does
   * not provide for matrix entries that will only be written into during
   * the elimination of hanging nodes or other constraints. They have to be
   * taken care of by a subsequent call to AffineConstraints::condense().
   * Alternatively, the constraints on degrees of freedom can already be
   * taken into account at the time of creating the sparsity pattern. For
   * this, pass the AffineConstraints object as the third argument to the
   * current function. No call to AffineConstraints::condense() is then
   * necessary. This process is explained in step-6, step-27, and other
   * tutorial programs.
   *
   * @param[in] keep_constrained_dofs In case the constraints are already
   * taken care of in this function by passing in a AffineConstraints object,
   * it is possible to abandon some off-diagonal entries in the sparsity
   * pattern if these entries will also not be written into during the actual
   * assembly of the matrix this sparsity pattern later serves. Specifically,
   * when using an assembly method that uses
   * AffineConstraints::distribute_local_to_global(), no entries will ever be
   * written into those matrix rows or columns that correspond to constrained
   * degrees of freedom. In such cases, you can set the argument @p
   * keep_constrained_dofs to @p false to avoid allocating these entries in
   * the sparsity pattern.
   *
   * @param[in] subdomain_id If specified, the sparsity pattern is built only
   * on cells that have a subdomain_id equal to the given argument. This is
   * useful in parallel contexts where the matrix and sparsity pattern (for
   * example a TrilinosWrappers::SparsityPattern) may be distributed and not
   * every MPI process needs to build the entire sparsity pattern; in that
   * case, it is sufficient if every process only builds that part of the
   * sparsity pattern that corresponds to the subdomain_id for which it is
   * responsible. This feature is used in step-32. (This argument is not
   * usually needed for objects of type parallel::distributed::Triangulation
   * because the current function only loops over locally owned cells anyway;
   * thus, this argument typically only makes sense if you want to use the
   * subdomain_id for anything other than indicating which processor owns a
   * cell, for example which geometric component of the domain a cell belongs
   * to.)
   *
   * @note The actual type of the sparsity pattern may be SparsityPattern,
   * DynamicSparsityPattern, BlockSparsityPattern,
   * BlockDynamicSparsityPattern, or any other class that satisfies similar
   * requirements. It is assumed that the size of the sparsity pattern matches
   * the number of degrees of freedom and that enough unused nonzero entries
   * are left to fill the sparsity pattern if the sparsity pattern is of
   * "static" kind (see
   * @ref Sparsity
   * for more information on what this means). The nonzero entries generated
   * by this function are added to possible previous content of the object,
   * i.e., previously added entries are not removed.
   *
   * @note If the sparsity pattern is represented by an object of type
   * SparsityPattern (as opposed to, for example, DynamicSparsityPattern), you
   * need to remember using SparsityPattern::compress() after generating the
   * pattern.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim, typename number = double>
  void
  make_sparsity_pattern(
    const DoFHandler<dim, spacedim> &dof_handler,
    SparsityPatternBase             &sparsity_pattern,
    const AffineConstraints<number> &constraints           = {},
    const bool                       keep_constrained_dofs = true,
    const types::subdomain_id subdomain_id = numbers::invalid_subdomain_id);

  /**
   * Compute which entries of a matrix built on the given @p dof_handler may
   * possibly be nonzero, and create a sparsity pattern object that represents
   * these nonzero locations.
   *
   * This function is a simple variation on the previous
   * make_sparsity_pattern() function (see there for a description of all of
   * the common arguments), but it provides functionality for vector finite
   * elements that allows to be more specific about which variables couple in
   * which equation.
   *
   * For example, if you wanted to solve the Stokes equations,
   *
   * @f{align*}{
   * -\Delta \mathbf u + \nabla p &= 0,\\ \text{div}\ u &= 0
   * @f}
   *
   * in two space dimensions, using stable Q2/Q1 mixed elements (using the
   * FESystem class), then you don't want all degrees of freedom to couple in
   * each equation. More specifically, in the first equation, only $u_x$ and
   * $p$ appear; in the second equation, only $u_y$ and $p$ appear; and in the
   * third equation, only $u_x$ and $u_y$ appear. (Note that this discussion
   * only talks about vector components of the solution variable and the
   * different equation, and has nothing to do with degrees of freedom, or in
   * fact with any kind of discretization.) We can describe this by the
   * following pattern of "couplings":
   *
   * @f[
   * \left[
   * \begin{array}{ccc}
   *   1 & 0 & 1 \\
   *   0 & 1 & 1 \\
   *   1 & 1 & 0
   * \end{array}
   * \right]
   * @f]
   *
   * where "1" indicates that two variables (i.e., vector components of the
   * FESystem) couple in the respective equation, and a "0" means no coupling.
   * These zeros imply that upon discretization via a standard finite element
   * formulation, we will not write entries into the matrix that, for example,
   * couple pressure test functions with pressure shape functions (and similar
   * for the other zeros above). It is then a waste to allocate memory for
   * these entries in the matrix and the sparsity pattern, and you can avoid
   * this by creating a mask such as the one above that describes this to the
   * (current) function that computes the sparsity pattern. As stated above,
   * the mask shown above refers to components of the composed FESystem,
   * rather than to degrees of freedom or shape functions.
   *
   * This function is designed to accept a coupling pattern, like the one
   * shown above, through the @p couplings parameter, which contains values of
   * type #Coupling. It builds the matrix structure just like the previous
   * function, but does not create matrix elements if not specified by the
   * coupling pattern. If the couplings are symmetric, then so will be the
   * resulting sparsity pattern.
   *
   * There is a complication if some or all of the shape functions of the
   * finite element in use are non-zero in more than one component (in deal.II
   * speak: they are
   * @ref GlossPrimitive "non-primitive finite elements").
   * In this case, the coupling element corresponding to the first non-zero
   * component is taken and additional ones for this component are ignored.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim, typename number = double>
  void
  make_sparsity_pattern(
    const DoFHandler<dim, spacedim> &dof_handler,
    const Table<2, Coupling>        &coupling,
    SparsityPatternBase             &sparsity_pattern,
    const AffineConstraints<number> &constraints           = {},
    const bool                       keep_constrained_dofs = true,
    const types::subdomain_id subdomain_id = numbers::invalid_subdomain_id);

  /**
   * Construct a sparsity pattern that allows coupling degrees of freedom on
   * two different but related meshes.
   *
   * The idea is that if the two given DoFHandler objects correspond to two
   * different meshes (and potentially to different finite elements used on
   * these cells), but that if the two triangulations they are based on are
   * derived from the same coarse mesh through hierarchical refinement, then
   * one may set up a problem where one would like to test shape functions
   * from one mesh against the shape functions from another mesh. In
   * particular, this means that shape functions from a cell on the first mesh
   * are tested against those on the second cell that are located on the
   * corresponding cell; this correspondence is something that the
   * IntergridMap class can determine.
   *
   * This function then constructs a sparsity pattern for which the degrees of
   * freedom that represent the rows come from the first given DoFHandler,
   * whereas the ones that correspond to columns come from the second
   * DoFHandler.
   */
  template <int dim, int spacedim>
  void
  make_sparsity_pattern(const DoFHandler<dim, spacedim> &dof_row,
                        const DoFHandler<dim, spacedim> &dof_col,
                        SparsityPatternBase             &sparsity);

  /**
   * Compute which entries of a matrix built on the given @p dof_handler may
   * possibly be nonzero, and create a sparsity pattern object that represents
   * these nonzero locations. This function is a variation of the
   * make_sparsity_pattern() functions above in that it assumes that the
   * bilinear form you want to use to generate the matrix also contains terms
   * that integrate over the <i>faces</i> between cells (i.e., it contains
   * "fluxes" between cells, explaining the name of the function).
   *
   * This function is useful for Discontinuous Galerkin methods where the
   * standard make_sparsity_pattern() function would only create nonzero
   * entries for all degrees of freedom on one cell coupling to all other
   * degrees of freedom on the same cell; however, in DG methods, all or some
   * degrees of freedom on each cell also couple to the degrees of freedom on
   * other cells connected to the current one by a common face. The current
   * function also creates the nonzero entries in the matrix resulting from
   * these additional couplings. In other words, this function computes a
   * strict super-set of nonzero entries compared to the work done by
   * make_sparsity_pattern().
   *
   * @param[in] dof_handler The DoFHandler object that describes which degrees
   * of freedom live on which cells.
   *
   * @param[out] sparsity_pattern The sparsity pattern to be filled with
   * entries.
   *
   * @note The actual type of the sparsity pattern may be SparsityPattern,
   * DynamicSparsityPattern, BlockSparsityPattern,
   * BlockDynamicSparsityPattern, or any other class that satisfies similar
   * requirements. It is assumed that the size of the sparsity pattern matches
   * the number of degrees of freedom and that enough unused nonzero entries
   * are left to fill the sparsity pattern if the sparsity pattern is of
   * "static" kind (see
   * @ref Sparsity
   * for more information on what this means). The nonzero entries generated
   * by this function are added to possible previous content of the object,
   * i.e., previously added entries are not removed.
   *
   * @note If the sparsity pattern is represented by an object of type
   * SparsityPattern (as opposed to, for example, DynamicSparsityPattern), you
   * need to remember using SparsityPattern::compress() after generating the
   * pattern.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim>
  void
  make_flux_sparsity_pattern(const DoFHandler<dim, spacedim> &dof_handler,
                             SparsityPatternBase             &sparsity_pattern);

  /**
   * This function does essentially the same as the other
   * make_flux_sparsity_pattern() function but allows the specification of a
   * number of additional arguments. These carry the same meaning as discussed
   * in the first make_sparsity_pattern() function above.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim, typename number>
  void
  make_flux_sparsity_pattern(
    const DoFHandler<dim, spacedim> &dof_handler,
    SparsityPatternBase             &sparsity_pattern,
    const AffineConstraints<number> &constraints,
    const bool                       keep_constrained_dofs = true,
    const types::subdomain_id subdomain_id = numbers::invalid_subdomain_id);


  /**
   * This function does essentially the same as the other
   * make_flux_sparsity_pattern() function but allows the specification of
   * coupling matrices that state which components of the solution variable
   * couple in each of the equations you are discretizing. This works in
   * complete analogy as discussed in the second make_sparsity_pattern()
   * function above.
   *
   * In fact, this function takes two such masks, one describing which
   * variables couple with each other in the cell integrals that make up your
   * bilinear form, and which variables couple with each other in the face
   * integrals. If you passed masks consisting of only 1s to both of these,
   * then you would get the same sparsity pattern as if you had called the
   * first of the make_flux_sparsity_pattern() functions above. By setting
   * some of the entries of these masks to zeros, you can get a sparser
   * sparsity pattern.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim>
  void
  make_flux_sparsity_pattern(
    const DoFHandler<dim, spacedim> &dof,
    SparsityPatternBase             &sparsity,
    const Table<2, Coupling>        &cell_integrals_mask,
    const Table<2, Coupling>        &face_integrals_mask,
    const types::subdomain_id subdomain_id = numbers::invalid_subdomain_id);


  /**
   * This function does essentially the same as the previous
   * make_flux_sparsity_pattern() function but allows the application of an
   * AffineConstraints object. This is useful in the case where some
   * components of a finite element are continuous and some discontinuous,
   * allowing constraints to be imposed on the continuous part while also
   * building the flux terms needed for the discontinuous part.
   *
   * The optional @p face_has_flux_coupling can be used to specify on which
   * faces flux couplings occur. This allows for creating a sparser pattern when
   * using a bilinear form where flux terms only appear on a subset of the faces
   * in the triangulation. By default flux couplings are added over all internal
   * faces. @p face_has_flux_coupling should be a function that takes an
   * active_cell_iterator and a face index and should return true if there is a
   * flux coupling over the face. When using DoFHandler we could, for example,
   * use
   *
   * @code
   *  auto face_has_flux_coupling =
   *    [](const typename DoFHandler<dim>::active_cell_iterator &cell,
   *       const unsigned int                                    face_index) {
   *      const Point<dim> &face_center = cell->face(face_index)->center();
   *      return 0 < face_center[0];
   *    };
   * @endcode
   */
  template <int dim, int spacedim, typename number>
  void
  make_flux_sparsity_pattern(
    const DoFHandler<dim, spacedim> &dof,
    SparsityPatternBase             &sparsity,
    const AffineConstraints<number> &constraints,
    const bool                       keep_constrained_dofs,
    const Table<2, Coupling>        &couplings,
    const Table<2, Coupling>        &face_couplings,
    const types::subdomain_id        subdomain_id,
    const std::function<
      bool(const typename DoFHandler<dim, spacedim>::active_cell_iterator &,
           const unsigned int)> &face_has_flux_coupling =
      &internal::always_couple_on_faces<dim, spacedim>);

  /**
   * Create the sparsity pattern for boundary matrices. See the general
   * documentation of this class for more information.
   *
   * The function does essentially what the other make_sparsity_pattern()
   * functions do, but assumes that the bilinear form that is used to build
   * the matrix does not consist of domain integrals, but only of integrals
   * over the boundary of the domain.
   */
  template <int dim, int spacedim>
  void
  make_boundary_sparsity_pattern(
    const DoFHandler<dim, spacedim>            &dof,
    const std::vector<types::global_dof_index> &dof_to_boundary_mapping,
    SparsityPatternBase                        &sparsity_pattern);

  /**
   * This function is a variation of the previous
   * make_boundary_sparsity_pattern() function in which we assume that the
   * boundary integrals that will give rise to the matrix extends only over
   * those parts of the boundary whose boundary indicators are listed in the
   * @p boundary_ids argument to this function.
   *
   * This function could have been written by passing a @p set of boundary_id
   * numbers. However, most of the functions throughout deal.II dealing with
   * boundary indicators take a mapping of boundary indicators and the
   * corresponding boundary function, i.e., a std::map<types::boundary_id, const
   * Function<spacedim,number>*> argument. Correspondingly, this function does
   * the same, though the actual boundary function is ignored here.
   * (Consequently, if you don't have any such boundary functions, just create a
   * map with the boundary indicators you want and set the function pointers to
   * null pointers).
   */
  template <int dim, int spacedim, typename number>
  void
  make_boundary_sparsity_pattern(
    const DoFHandler<dim, spacedim> &dof,
    const std::map<types::boundary_id, const Function<spacedim, number> *>
                                               &boundary_ids,
    const std::vector<types::global_dof_index> &dof_to_boundary_mapping,
    SparsityPatternBase                        &sparsity);

  /**
   * @}
   */

  /**
   * @name Hanging nodes and other constraints
   * @{
   */

  /**
   * Compute the constraints resulting from the presence of hanging nodes.
   * Hanging nodes are best explained using a small picture:
   *
   * @image html hanging_nodes.png
   *
   * In order to make a finite element function globally continuous, we have
   * to make sure that the dark red nodes have values that are compatible with
   * the adjacent yellow nodes, so that the function has no jump when coming
   * from the small cells to the large one at the top right. We therefore have
   * to add conditions that constrain those "hanging nodes".
   *
   * The object into which these are inserted is later used to condense the
   * global system matrix and right hand side, and to extend the solution
   * vectors from the true degrees of freedom also to the constraint nodes.
   * This function is explained in detail in the
   * step-6
   * tutorial program and is used in almost all following programs as well.
   *
   * This function does not clear the AffineConstraints object before use, in
   * order to allow adding constraints from different sources to the same
   * object. You therefore need to make sure it contains only constraints you
   * still want; otherwise call the AffineConstraints::clear() function.
   * Since this function does not check if it would add cycles in
   * @p constraints, it is recommended to call this function prior to other
   * functions that constrain DoFs with respect to others such as
   * make_periodicity_constraints().
   * This function does not close the object since you may want to
   * enter other constraints later on yourself.
   *
   * Using a DoFHandler with hp-capabilities, we consider constraints due to
   * different finite elements used on two sides of a face between cells as
   * hanging nodes as well. In other words, in hp-mode, this function computes
   * all constraints due to differing mesh sizes (h) or polynomial degrees (p)
   * between adjacent cells.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim, typename number>
  void
  make_hanging_node_constraints(const DoFHandler<dim, spacedim> &dof_handler,
                                AffineConstraints<number>       &constraints);

  /**
   * This function is used when different variables in a problem are
   * discretized on different grids, where one grid is strictly coarser than
   * the other. An example are optimization problems where the control
   * variable is often discretized on a coarser mesh than the state variable.
   *
   * The function's result can be stated as follows mathematically: Let ${\cal
   * T}_0$ and ${\cal T}_1$ be two meshes where ${\cal T}_1$ results from
   * ${\cal T}_0$ strictly by refining or leaving alone the cells of ${\cal
   * T}_0$. Using the same finite element on both, there are function spaces
   * ${\cal V}_0$ and ${\cal V}_1$ associated with these meshes. Then every
   * function $v_0 \in {\cal V}_0$ can of course also be represented exactly
   * in ${\cal V}_1$ since by construction ${\cal V}_0 \subset {\cal V}_1$.
   * However, not every function in ${\cal V}_1$ can be expressed as a linear
   * combination of the shape functions of ${\cal V}_0$. The functions that
   * can be represented lie in a homogeneous subspace of ${\cal V}_1$ (namely,
   * ${\cal V}_0$, of course) and this subspace can be represented by a linear
   * constraint of the form $CV=0$ where $V$ is the vector of nodal values of
   * functions $v\in {\cal V}_1$. In other words, every function $v_h=\sum_j
   * V_j \varphi_j^{(1)} \in {\cal V}_1$ that also satisfies $v_h\in {\cal
   * V}_0$ automatically satisfies $CV=0$. This function computes the matrix
   * $C$ in the form of a AffineConstraints object.
   *
   * The construction of these constraints is done as follows: for each of the
   * degrees of freedom (i.e. shape functions) on the coarse grid, we compute
   * its representation on the fine grid, i.e. how the linear combination of
   * shape functions on the fine grid looks like that resembles the shape
   * function on the coarse grid. From this information, we can then compute
   * the constraints which have to hold if a solution of a linear equation on
   * the fine grid shall be representable on the coarse grid. The exact
   * algorithm how these constraints can be computed is rather complicated and
   * is best understood by reading the source code, which contains many
   * comments.
   *
   * The use of this function is as follows: it accepts as parameters two DoF
   * Handlers, the first of which refers to the coarse grid and the second of
   * which is the fine grid. On both, a finite element is represented by the
   * DoF handler objects, which will usually have several vector components,
   * which may belong to different base elements. The second and fourth
   * parameter of this function therefore state which vector component on the
   * coarse grid shall be used to restrict the stated component on the fine
   * grid. The finite element used for the respective components on the two
   * grids needs to be the same. An example may clarify this: consider an
   * optimization problem with controls $q$ discretized on a coarse mesh and a
   * state variable $u$ (and corresponding Lagrange multiplier $\lambda$)
   * discretized on the fine mesh. These are discretized using piecewise
   * constant discontinuous, continuous linear, and continuous linear
   * elements, respectively. Only the parameter $q$ is represented on the
   * coarse grid, thus the DoFHandler object on the coarse grid represents
   * only one variable, discretized using piecewise constant discontinuous
   * elements. Then, the parameter denoting the vector component on the coarse
   * grid would be zero (the only possible choice, since the variable on the
   * coarse grid is scalar). If the ordering of variables in the fine mesh
   * FESystem is $u, q, \lambda$, then the fourth argument of the function
   * corresponding to the vector component would be one (corresponding to the
   * variable $q$; zero would be $u$, two would be $\lambda$).
   *
   * The function also requires an object of type IntergridMap representing
   * how to get from the coarse mesh cells to the corresponding cells on the
   * fine mesh. This could in principle be generated by the function itself
   * from the two DoFHandler objects, but since it is probably available
   * anyway in programs that use different meshes, the function simply takes
   * it as an argument.
   *
   * The computed constraints are entered into a variable of type
   * AffineConstraints; previous contents are not deleted.
   */
  template <int dim, int spacedim>
  void
  compute_intergrid_constraints(
    const DoFHandler<dim, spacedim>               &coarse_grid,
    const unsigned int                             coarse_component,
    const DoFHandler<dim, spacedim>               &fine_grid,
    const unsigned int                             fine_component,
    const InterGridMap<DoFHandler<dim, spacedim>> &coarse_to_fine_grid_map,
    AffineConstraints<double>                     &constraints);


  /**
   * This function generates a matrix such that when a vector of data with as
   * many elements as there are degrees of freedom of this component on the
   * coarse grid is multiplied to this matrix, we obtain a vector with as many
   * elements as there are global degrees of freedom on the fine grid. All the
   * elements of the other vector components of the finite element fields on
   * the fine grid are not touched.
   *
   * Triangulation of the fine grid can be distributed. When called in
   * parallel, each process has to have a copy of the coarse grid. In this
   * case, function returns transfer representation for a set of locally owned
   * cells.
   *
   * The output of this function is a compressed format that can be used to
   * construct corresponding sparse transfer matrix.
   */
  template <int dim, int spacedim>
  void
  compute_intergrid_transfer_representation(
    const DoFHandler<dim, spacedim>               &coarse_grid,
    const unsigned int                             coarse_component,
    const DoFHandler<dim, spacedim>               &fine_grid,
    const unsigned int                             fine_component,
    const InterGridMap<DoFHandler<dim, spacedim>> &coarse_to_fine_grid_map,
    std::vector<std::map<types::global_dof_index, float>>
      &transfer_representation);

  /**
   * @}
   */


  /**
   * @name Periodic boundary conditions
   * @{
   */

  /**
   * Insert the (algebraic) constraints due to periodic boundary conditions
   * into an AffineConstraints object @p constraints.
   *
   * Given a pair of not necessarily active boundary faces @p face_1 and @p
   * face_2, this functions constrains all DoFs associated with the boundary
   * described by @p face_1 to the respective DoFs of the boundary described
   * by @p face_2. More precisely:
   *
   * If @p face_1 and @p face_2 are both active faces it adds the DoFs of @p
   * face_1 to the list of constrained DoFs in @p constraints and adds
   * entries to constrain them to the corresponding values of the DoFs on @p
   * face_2. This happens on a purely algebraic level, meaning, the global DoF
   * with (local face) index <tt>i</tt> on @p face_1 gets constraint to the
   * DoF with (local face) index <tt>i</tt> on @p face_2 (possibly corrected
   * for orientation, see below).
   *
   * Otherwise, if @p face_1 and @p face_2 are not active faces, this function
   * loops recursively over the children of @p face_1 and @p face_2. If only
   * one of the two faces is active, then we recursively iterate over the
   * children of the non-active ones and make sure that the solution function
   * on the refined side equals that on the non-refined face in much the same
   * way as we enforce hanging node constraints at places where differently
   * refined cells come together. (However, unlike hanging nodes, we do not
   * enforce the requirement that there be only a difference of one refinement
   * level between the two sides of the domain you would like to be periodic).
   *
   * This routine only constrains DoFs that are not already constrained. If
   * this routine encounters a DoF that already is constrained (for instance
   * by Dirichlet boundary conditions), the old setting of the constraint
   * (dofs the entry is constrained to, inhomogeneities) is kept and nothing
   * happens.
   *
   * The flags in the @p component_mask (see
   * @ref GlossComponentMask)
   * denote which components of the finite element space shall be constrained
   * with periodic boundary conditions. If it is left as specified by the
   * default value all components are constrained. If it is different from the
   * default value, it is assumed that the number of entries equals the number
   * of components of the finite element. This can be used to enforce
   * periodicity in only one variable in a system of equations.
   *
   * Here, @p combined_orientation is the relative orientation of @p face_1 with
   * respect to @p face_2. This is typically computed by
   * GridTools::orthogonal_equality().
   *
   * Optionally a matrix @p matrix along with a std::vector @p
   * first_vector_components can be specified that describes how DoFs on @p
   * face_1 should be modified prior to constraining to the DoFs of @p face_2.
   * Here, two declarations are possible: If the std::vector @p
   * first_vector_components is non empty the matrix is interpreted as a @p
   * dim $\times$ @p dim rotation matrix that is applied to all vector valued
   * blocks listed in @p first_vector_components of the FESystem. If @p
   * first_vector_components is empty the matrix is interpreted as an
   * interpolation matrix with size no_face_dofs $\times$ no_face_dofs.
   *
   * This function makes sure that identity constraints don't create cycles
   * in @p constraints.
   *
   * @p periodicity_factor can be used to implement Bloch periodic conditions
   * (a.k.a. phase shift periodic conditions) of the form
   * $\psi(\mathbf{r})=e^{-i\mathbf{k}\cdot\mathbf{r}}u(\mathbf{r})$
   * where $u$ is periodic with the same periodicity as the crystal lattice and
   * $\mathbf{k}$ is the wavevector, see
   * [https://en.wikipedia.org/wiki/Bloch_wave](https://en.wikipedia.org/wiki/Bloch_wave).
   * The solution at @p face_2 is equal to the solution at @p face_1 times
   * @p periodicity_factor. For example, if the solution at @p face_1 is
   * $\psi(0)$ and $\mathbf{d}$ is the corresponding point on @p face_2, then
   * the solution at @p face_2 should be
   * $\psi(d) = \psi(0)e^{-i \mathbf{k}\cdot \mathbf{d}}$. This condition can be
   * implemented using
   * $\mathrm{periodicity\_factor}=e^{-i \mathbf{k}\cdot \mathbf{d}}$.
   *
   * Detailed information can be found in the see
   * @ref GlossPeriodicConstraints "Glossary entry on periodic boundary conditions".
   */
  template <typename FaceIterator, typename number>
  void
  make_periodicity_constraints(
    const FaceIterator                             &face_1,
    const std_cxx20::type_identity_t<FaceIterator> &face_2,
    AffineConstraints<number>                      &constraints,
    const ComponentMask                            &component_mask = {},
    const types::geometric_orientation              combined_orientation =
      numbers::default_geometric_orientation,
    const FullMatrix<double>        &matrix = FullMatrix<double>(),
    const std::vector<unsigned int> &first_vector_components =
      std::vector<unsigned int>(),
    const number periodicity_factor = 1.);


  /**
   * Insert the (algebraic) constraints due to periodic boundary conditions
   * into an AffineConstraints object @p constraints.
   *
   * This is the main high level interface for above low level variant of
   * make_periodicity_constraints(). It takes a std::vector @p periodic_faces
   * as argument and applies above make_periodicity_constraints() on each
   * entry. @p periodic_faces can be created by
   * GridTools::collect_periodic_faces.
   *
   * @note For DoFHandler objects that are built on a
   * parallel::distributed::Triangulation object
   * parallel::distributed::Triangulation::add_periodicity has to be called
   * before calling this function..
   *
   * @see
   * @ref GlossPeriodicConstraints "Glossary entry on periodic boundary conditions"
   * and step-45 for further information.
   */
  template <int dim, int spacedim, typename number>
  void
  make_periodicity_constraints(
    const std::vector<GridTools::PeriodicFacePair<
      typename DoFHandler<dim, spacedim>::cell_iterator>> &periodic_faces,
    AffineConstraints<number>                             &constraints,
    const ComponentMask                                   &component_mask = {},
    const std::vector<unsigned int> &first_vector_components =
      std::vector<unsigned int>(),
    const number periodicity_factor = 1.);



  /**
   * Insert the (algebraic) constraints due to periodic boundary conditions
   * into a AffineConstraints @p constraints.
   *
   * This function serves as a high level interface for the
   * make_periodicity_constraints() function.
   *
   * Define a 'first' boundary as all boundary faces having boundary_id @p
   * b_id1 and a 'second' boundary consisting of all faces belonging to @p
   * b_id2.
   *
   * This function tries to match all faces belonging to the first boundary
   * with faces belonging to the second boundary with the help of
   * orthogonal_equality(). More precisely, faces with coordinates only
   * differing in the @p direction component are identified.
   *
   * If this matching is successful it constrains all DoFs associated with the
   * 'first' boundary to the respective DoFs of the 'second' boundary
   * respecting the relative orientation of the two faces.
   *
   * @note This function is a convenience wrapper. It internally calls
   * GridTools::collect_periodic_faces() with the supplied parameters and
   * feeds the output to above make_periodicity_constraints() variant. If you
   * need more functionality use GridTools::collect_periodic_faces() directly.
   *
   * @see
   * @ref GlossPeriodicConstraints "Glossary entry on periodic boundary conditions"
   * for further information.
   */
  template <int dim, int spacedim, typename number>
  void
  make_periodicity_constraints(const DoFHandler<dim, spacedim> &dof_handler,
                               const types::boundary_id         b_id1,
                               const types::boundary_id         b_id2,
                               const unsigned int               direction,
                               AffineConstraints<number>       &constraints,
                               const ComponentMask &component_mask     = {},
                               const number         periodicity_factor = 1.);



  /**
   * Instead of defining a 'first' and 'second' boundary with the help of two
   * boundary_ids this function defines a 'left' boundary as all faces with
   * local face index <code>2*dimension</code> and boundary indicator @p b_id
   * and, similarly, a 'right' boundary consisting of all face with local face
   * index <code>2*dimension+1</code> and boundary indicator @p b_id. Faces with
   * coordinates only differing in the @p direction component are identified.
   *
   * @warning This version of make_periodicity_constraints will not work on
   * meshes with cells not in
   * @ref GlossCombinedOrientation "the default orientation".
   *
   * @note This function is a convenience wrapper. It internally calls
   * GridTools::collect_periodic_faces() with the supplied parameters and
   * feeds the output to above make_periodicity_constraints() variant. If you
   * need more functionality use GridTools::collect_periodic_faces() directly.
   *
   * @see
   * @ref GlossPeriodicConstraints "Glossary entry on periodic boundary conditions"
   * for further information.
   */
  template <int dim, int spacedim, typename number>
  void
  make_periodicity_constraints(const DoFHandler<dim, spacedim> &dof_handler,
                               const types::boundary_id         b_id,
                               const unsigned int               direction,
                               AffineConstraints<number>       &constraints,
                               const ComponentMask &component_mask     = {},
                               const number         periodicity_factor = 1.);

  namespace internal
  {
    /**
     * This function is internally used in make_periodicity_constraints().
     * Enter constraints for periodicity into the given AffineConstraints
     * object.
     *
     * This function works both on 1) an active mesh
     * (`level == numbers::invalid_unsigned_int`) and on 2) multigrid levels.
     *
     * In the case of an active mesh, this function is called when at least
     * one of the two face iterators corresponds to an active object
     * without further children. Furthermore, @p face_1 is supposed to be active.
     *
     * The matrix @p transformation maps degrees of freedom from one face
     * to another. If the DoFs on the two faces are supposed to match exactly,
     * then the matrix so provided will be the identity matrix. if face 2 is
     * once refined from face 1, then the matrix needs to be the interpolation
     * matrix from a face to this particular child
     *
     * @note We have to be careful not to accidentally create constraint
     * cycles when adding periodic constraints: For example, as the
     * corresponding testcase bits/periodicity_05 demonstrates, we can
     * occasionally get into trouble if we already have the constraint
     * x1 == x2 and want to insert x2 == x1. We avoid this by skipping
     * such "identity constraints" if the opposite constraint already
     * exists.
     */
    template <typename FaceIterator, typename number>
    void
    set_periodicity_constraints(
      const FaceIterator                             &face_1,
      const std_cxx20::type_identity_t<FaceIterator> &face_2,
      const FullMatrix<double>                       &transformation,
      AffineConstraints<number>                      &affine_constraints,
      const ComponentMask                            &component_mask,
      const types::geometric_orientation              combined_orientation,
      const number                                    periodicity_factor,
      const unsigned int level = numbers::invalid_unsigned_int);
  } // namespace internal

  /**
   * @}
   */

  /**
   * @name Identifying subsets of degrees of freedom with particular properties
   * @{
   */

  /**
   * Return an IndexSet describing all dofs that will be constrained by
   * interface constraints, i.e. all hanging nodes.
   *
   * In case of a parallel::shared::Triangulation or a
   * parallel::distributed::Triangulation only locally relevant dofs are
   * considered.
   */
  template <int dim, int spacedim>
  IndexSet
  extract_hanging_node_dofs(const DoFHandler<dim, spacedim> &dof_handler);

  /**
   * Extract the (locally owned) indices of the degrees of freedom belonging to
   * certain vector components of a vector-valued finite element. The
   * @p component_mask defines which components or blocks of an FESystem or
   * vector-valued element are to be extracted
   * from the DoFHandler @p dof. The entries in the output object then
   * correspond to degrees of freedom belonging to these
   * components.
   *
   * If the finite element under consideration is not primitive, i.e., some or
   * all of its shape functions are non-zero in more than one vector component
   * (which holds, for example, for FE_Nedelec or FE_RaviartThomas elements),
   * then shape functions cannot be associated with a single vector component.
   * In this case, if <em>one</em> shape vector component of this element is
   * flagged in @p component_mask (see
   * @ref GlossComponentMask),
   * then this is equivalent to selecting <em>all</em> vector components
   * corresponding to this non-primitive base element.
   *
   * @param[in] dof_handler The DoFHandler whose enumerated degrees of freedom
   *   are to be filtered by this function.
   * @param[in] component_mask A mask that states which components you want
   *   to select. The size of this mask must be compatible with the number of
   *   components in the FiniteElement used by the @p dof_handler. A common
   *   way to create this kind of mask is using extractors for whatever
   *   vector component you want to extract, and using it with
   *   FiniteElement::component_mask(). See
   *   @ref GlossComponentMask "the glossary entry on component masks"
   *   for more information.
   * @return An IndexSet object that will contain exactly those entries that
   *   (i) correspond to degrees of freedom selected by the mask above, and
   *   (ii) are locally owned. The size of the index set is equal to the global
   *   number of degrees of freedom. Note that the resulting object is always
   *   a subset of what DoFHandler::locally_owned_dofs() returns.
   */
  template <int dim, int spacedim>
  IndexSet
  extract_dofs(const DoFHandler<dim, spacedim> &dof_handler,
               const ComponentMask             &component_mask);

  /**
   * This function is the equivalent to the DoFTools::extract_dofs() functions
   * above except that the selection of which degrees of freedom to extract is
   * not done based on components (see
   * @ref GlossComponent)
   * but instead based on whether they are part of a particular block (see
   * @ref GlossBlock).
   * Consequently, the second argument is not a ComponentMask but a BlockMask
   * object.
   *
   * @param[in] dof_handler The DoFHandler whose enumerated degrees of freedom
   *   are to be filtered by this function.
   * @param[in] block_mask A mask that states which blocks you want
   *   to select. The size of this mask must be compatible with the number of
   *   blocks in the FiniteElement used by the @p dof_handler. See
   *   @ref GlossBlockMask "the glossary entry on block masks"
   *   for more information.
   * @return An IndexSet object that will contain exactly those entries that
   *   (i) correspond to degrees of freedom selected by the mask above, and
   *   (ii) are locally owned. The size of the index set is equal to the global
   *   number of degrees of freedom. Note that the resulting object is always
   *   a subset of what DoFHandler::locally_owned_dofs() returns.
   */
  template <int dim, int spacedim>
  IndexSet
  extract_dofs(const DoFHandler<dim, spacedim> &dof_handler,
               const BlockMask                 &block_mask);

  /**
   * Do the same thing as the corresponding extract_dofs() function for one
   * level of a multi-grid DoF numbering.
   */
  template <int dim, int spacedim>
  void
  extract_level_dofs(const unsigned int               level,
                     const DoFHandler<dim, spacedim> &dof,
                     const ComponentMask             &component_mask,
                     std::vector<bool>               &selected_dofs);

  /**
   * Do the same thing as the corresponding extract_dofs() function for one
   * level of a multi-grid DoF numbering.
   */
  template <int dim, int spacedim>
  void
  extract_level_dofs(const unsigned int               level,
                     const DoFHandler<dim, spacedim> &dof,
                     const BlockMask                 &component_mask,
                     std::vector<bool>               &selected_dofs);

  /**
   * Extract all degrees of freedom which are at the boundary and belong to
   * specified components of the solution. The function returns its results in
   * the form of an IndexSet that contains those entries that correspond to
   * these selected degrees of freedom, i.e., which are at the boundary and
   * belong to one of the selected components.
   *
   * By specifying the @p boundary_ids variable, you can select which boundary
   * indicators the faces have to have on which the degrees of freedom are
   * located that shall be extracted. If it is an empty list (the default), then
   * all boundary indicators are accepted.
   *
   * This function is used in step-11 and step-15, for example.
   *
   * @note If the DoFHandler object is defined on a
   * parallel Triangulation object, then the computed index set
   * will contain only those degrees of freedom on the boundary that belong to
   * the locally relevant set (see
   * @ref GlossLocallyRelevantDof "locally relevant DoFs"), i.e., the function
   * only considers faces of locally owned and ghost cells, but not of
   * artificial cells.
   *
   * @param[in] dof_handler The object that describes which degrees of freedom
   * live on which cell.
   * @param[in] component_mask A mask denoting the vector components of the
   * finite element that should be considered (see also
   * @ref GlossComponentMask). If left at the default, the component mask
   * indicates that all vector components of the finite element should be
   * considered.
   * @param[in] boundary_ids If empty, this function extracts the indices of the
   * degrees of freedom for all parts of the boundary. If it is a non-empty
   * list, then the function only considers boundary faces with the boundary
   * indicators listed in this argument.
   * @return The IndexSet object that
   * will contain the indices of degrees of freedom that are located on the
   * boundary (and correspond to the selected vector components and boundary
   * indicators, depending on the values of the @p component_mask and @p
   * boundary_ids arguments).
   *
   * @see
   * @ref GlossBoundaryIndicator "Glossary entry on boundary indicators"
   */
  template <int dim, int spacedim>
  IndexSet
  extract_boundary_dofs(const DoFHandler<dim, spacedim>    &dof_handler,
                        const ComponentMask                &component_mask = {},
                        const std::set<types::boundary_id> &boundary_ids = {});

  /**
   * This function is similar to the extract_boundary_dofs() function but it
   * extracts those degrees of freedom whose shape functions are nonzero on at
   * least part of the selected boundary. For continuous elements, this is
   * exactly the set of shape functions whose degrees of freedom are defined
   * on boundary faces. On the other hand, if the finite element in used is a
   * discontinuous element, all degrees of freedom are defined in the inside
   * of cells and consequently none would be boundary degrees of freedom.
   * Several of those would have shape functions that are nonzero on the
   * boundary, however. This function therefore extracts all those for which
   * the FiniteElement::has_support_on_face function says that it is nonzero
   * on any face on one of the selected boundary parts.
   *
   * @see
   * @ref GlossBoundaryIndicator "Glossary entry on boundary indicators"
   */
  template <int dim, int spacedim>
  void
  extract_dofs_with_support_on_boundary(
    const DoFHandler<dim, spacedim>    &dof_handler,
    const ComponentMask                &component_mask,
    std::vector<bool>                  &selected_dofs,
    const std::set<types::boundary_id> &boundary_ids =
      std::set<types::boundary_id>());

  /**
   * Extract all indices of shape functions such that their support is entirely
   * contained within the cells for which the @p predicate is <code>true</code>.
   * The result is returned as an IndexSet.
   *
   * Consider the following FE space where predicate returns <code>true</code>
   * for all cells on the left half of the domain:
   *
   * @image html extract_dofs_with_support_contained_within.png
   *
   * This functions will return the union of all DoF indices on those cells
   * minus DoF 11, 13, 2 and 0; the result will be <code>[9,10], 12,
   * [14,38]</code>. In the image above the returned DoFs are separated from the
   * rest by the red line
   *
   * Essentially, the question this functions answers is the following:
   * Given a subdomain with associated DoFs, what is the largest subset of
   * these DoFs that are allowed to be non-zero such that after calling
   * AffineConstraints::distribute() the resulting solution vector will have
   * support only within the given domain. Here, @p constraints is the
   * AffineConstraints container containing hanging nodes constraints.
   *
   * In case of parallel::distributed::Triangulation @p predicate will be called
   * only for locally owned and ghost cells. The resulting index set may contain
   * DoFs that are associated with the locally owned or ghost cells, but are not
   * owned by the current MPI core.
   */
  template <int dim, int spacedim, typename number = double>
  IndexSet
  extract_dofs_with_support_contained_within(
    const DoFHandler<dim, spacedim> &dof_handler,
    const std::function<
      bool(const typename DoFHandler<dim, spacedim>::active_cell_iterator &)>
                                    &predicate,
    const AffineConstraints<number> &constraints = {});

  /**
   * Extract a vector that represents the constant modes of the DoFHandler for
   * the components chosen by <tt>component_mask</tt> (see
   * @ref GlossComponentMask).
   * The constant modes on a discretization are the null space of a Laplace
   * operator on the selected components with Neumann boundary conditions
   * applied. The null space is a necessary ingredient for obtaining a good
   * AMG preconditioner when using the class
   * TrilinosWrappers::PreconditionAMG.  Since the ML AMG package only works
   * on algebraic properties of the respective matrix, it has no chance to
   * detect whether the matrix comes from a scalar or a vector valued problem.
   * However, a near null space supplies exactly the needed information about
   * the components' placement of vector components within the matrix. The null
   * space (or rather, the constant modes) is provided by the finite element
   * underlying the given DoFHandler and for most elements, the null space
   * will consist of as many vectors as there are `true` arguments in
   * <tt>component_mask</tt> (see
   * @ref GlossComponentMask),
   * each of which will be one in one vector component and zero in all others.
   * However, the representation of the constant function for e.g. FE_DGP is
   * different (the first component on each element one, all other components
   * zero), and some scalar elements may even have two constant modes
   * (FE_Q_DG0). Therefore, we store this object in a vector of vectors, where
   * the outer vector contains the collection of the actual constant modes on
   * the DoFHandler. Each inner vector has as many components as there are
   * (locally owned) degrees of freedom in the selected components. Note that
   * any matrix associated with this null space must have been constructed
   * using the same <tt>component_mask</tt> argument, since the numbering of
   * DoFs is done relative to the selected dofs, not to all dofs.
   *
   * The main reason for this program is the use of the null space with the
   * AMG preconditioner.
   *
   * This function is used in step-31, step-32, and step-42, for example.
   */
  template <int dim, int spacedim>
  std::vector<std::vector<bool>>
  extract_constant_modes(const DoFHandler<dim, spacedim> &dof_handler,
                         const ComponentMask             &component_mask = {});

  /**
   * Same as above.
   *
   * @deprecated
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED_WITH_COMMENT(
    "Use the other function that returns the constant modes by value, rather than via an argument.")
  void extract_constant_modes(const DoFHandler<dim, spacedim> &dof_handler,
                              const ComponentMask             &component_mask,
                              std::vector<std::vector<bool>>  &constant_modes);

  /**
   * Same as above but for multigrid levels.
   */
  template <int dim, int spacedim>
  std::vector<std::vector<bool>>
  extract_level_constant_modes(const unsigned int               level,
                               const DoFHandler<dim, spacedim> &dof_handler,
                               const ComponentMask &component_mask = {});

  /**
   * Same as above.
   *
   * @deprecated
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED_WITH_COMMENT(
    "Use the other function that returns the constant modes by value, rather than via an argument.")
  void extract_level_constant_modes(
    const unsigned int               level,
    const DoFHandler<dim, spacedim> &dof_handler,
    const ComponentMask             &component_mask,
    std::vector<std::vector<bool>>  &constant_modes);

  /**
   * Same as the above but additional to the translational modes also
   * the rotational modes are added, needed, e.g., to set up AMG for solving
   * elasticity problems.
   */
  template <int dim, int spacedim>
  std::vector<std::vector<double>>
  extract_rigid_body_modes(const Mapping<dim, spacedim>    &mapping,
                           const DoFHandler<dim, spacedim> &dof_handler,
                           const ComponentMask &component_mask = {});

  /**
   * Same as above but for multigrid levels.
   */
  template <int dim, int spacedim>
  std::vector<std::vector<double>>
  extract_level_rigid_body_modes(const unsigned int               level,
                                 const Mapping<dim, spacedim>    &mapping,
                                 const DoFHandler<dim, spacedim> &dof_handler,
                                 const ComponentMask &component_mask = {});
  /** @} */

  /**
   * @name Coupling between DoFHandler objects on different dimensions
   * @{
   */

  /**
   * This function generates a mapping of codimension-1 active DoFHandler cell
   * iterators to codimension-0 cells and face indices, for DoFHandler objects
   * built on top of the boundary of a given `Triangulation<spacedim>`.
   *
   * If you need to couple a PDE defined on the surface of an existing
   * Triangulation (as in step-38) with the PDE defined on the bulk (say, for
   * example, a hyper ball and its boundary), the information that is returned
   * by the function GridGenerator::extract_boundary_mesh() is not enough, since
   * you need to build FEValues objects on active cell iterators of the
   * DoFHandler defined on the surface mesh, and FEFaceValues objects on face
   * iterators of the DoFHandler defined on the bulk mesh. This second step
   * requires knowledge of the bulk cell iterator and of the corresponding face
   * index.
   *
   * This function examines the map `c1_to_c0` returned by
   * GridGenerator::extract_boundary_mesh() (when used with two Triangulation
   * objects as input), and associates to each active cell iterator of the
   * `c1_dh` DoFHandler that is also contained in the map `c1_to_c0`, a pair of
   * corresponding active bulk cell iterator of `c0_dh`, and face index
   * corresponding to the cell iterator on the surface mesh.
   *
   * An example usage of this function is the following:
   *
   * @code
   * Triangulation<dim>          triangulation;
   * Triangulation<dim - 1, dim> surface_triangulation;
   *
   * FE_Q<dim>       fe(1);
   * DoFHandler<dim> dof_handler(triangulation);
   *
   * FE_Q<dim - 1, dim>       surface_fe(1);
   * DoFHandler<dim - 1, dim> surface_dof_handler(surface_triangulation);
   *
   * GridGenerator::half_hyper_ball(triangulation);
   * triangulation.refine_global(4);
   *
   * surface_triangulation.set_manifold(0, SphericalManifold<dim - 1, dim>());
   * const auto surface_to_bulk_map =
   *   GridGenerator::extract_boundary_mesh(triangulation,
   *                                        surface_triangulation,
   *                                        {0});
   *
   * dof_handler.distribute_dofs(fe);
   * surface_dof_handler.distribute_dofs(surface_fe);
   *
   * // Extract the mapping between surface and bulk degrees of freedom:
   * const auto surface_to_bulk_dof_iterator_map =
   *   DoFTools::map_boundary_to_bulk_dof_iterators(surface_to_bulk_map,
   *                                                dof_handler,
   *                                                surface_dof_handler);
   *
   * // Loop over the map, and print some information:
   * for (const auto &p : surface_to_bulk_dof_iterator_map)
   *   {
   *     const auto &surface_cell = p.first;
   *     const auto &bulk_cell    = p.second.first;
   *     const auto &bulk_face    = p.second.second;
   *     deallog << "Surface cell " << surface_cell << " coincides with face "
   *             << bulk_face << " of bulk cell " << bulk_cell << std::endl;
   *   }
   * @endcode
   *
   * \tparam dim The dimension of the codimension-0 mesh.
   *
   * \tparam spacedim The dimension of the underlying space.
   *
   * \param[in] c1_to_c0 A map from codimension-1 triangulation cell iterators
   * to codimension-0 face iterators, as generated by the
   * GridGenerators::extract_boundary_mesh() function.
   *
   * \param[in] c0_dh The DoFHandler object of the codimension-0 mesh.
   *
   * \param[in] c1_dh The DoFHandler object of the codimension-1 mesh.
   *
   * \return A std::map object that maps codimension-1 active DoFHandler cell
   * iterators to a pair consisting of the corresponding codimension-0 cell
   * iterator and face index.
   */
  template <int dim, int spacedim>
  std::map<typename DoFHandler<dim - 1, spacedim>::active_cell_iterator,
           std::pair<typename DoFHandler<dim, spacedim>::active_cell_iterator,
                     unsigned int>>
  map_boundary_to_bulk_dof_iterators(
    const std::map<typename Triangulation<dim - 1, spacedim>::cell_iterator,
                   typename Triangulation<dim, spacedim>::face_iterator>
                                        &c1_to_c0,
    const DoFHandler<dim, spacedim>     &c0_dh,
    const DoFHandler<dim - 1, spacedim> &c1_dh);

  /**
   * @}
   */

  /**
   * @name Parallelization and domain decomposition
   * @{
   */
  /**
   * Flag all those degrees of freedom which are on cells with the given
   * subdomain id. Note that DoFs on faces can belong to cells with differing
   * subdomain ids, so the sets of flagged degrees of freedom are not mutually
   * exclusive for different subdomain ids.
   *
   * If you want to get a unique association of degree of freedom with
   * subdomains, use the @p get_subdomain_association function.
   */
  template <int dim, int spacedim>
  void
  extract_subdomain_dofs(const DoFHandler<dim, spacedim> &dof_handler,
                         const types::subdomain_id        subdomain_id,
                         std::vector<bool>               &selected_dofs);

  /**
   * Extract the set of global DoF indices that are active on the current
   * DoFHandler. For regular DoFHandlers, these are all DoF indices, but for
   * DoFHandler objects built on parallel::distributed::Triangulation this set
   * is a superset of DoFHandler::locally_owned_dofs() and contains all DoF
   * indices that live on all locally owned cells (including on the interface
   * to ghost cells). However, it does not contain the DoF indices that are
   * exclusively defined on ghost or artificial cells (see
   * @ref GlossArtificialCell "the glossary").
   *
   * The degrees of freedom identified by this function equal those obtained
   * from the dof_indices_with_subdomain_association() function when called
   * with the locally owned subdomain id.
   */
  template <int dim, int spacedim>
  IndexSet
  extract_locally_active_dofs(const DoFHandler<dim, spacedim> &dof_handler);

  /**
   * Extract the set of global DoF indices that are active on the current
   * DoFHandler. For regular DoFHandlers, these are all DoF indices, but for
   * DoFHandler objects built on parallel::distributed::Triangulation this set
   * is a superset of DoFHandler::locally_owned_dofs() and contains all DoF
   * indices that live on all locally owned cells (including on the interface
   * to ghost cells). However, it does not contain the DoF indices that are
   * exclusively defined on ghost or artificial cells (see
   * @ref GlossArtificialCell "the glossary").
   *
   * The degrees of freedom identified by this function equal those obtained
   * from the dof_indices_with_subdomain_association() function when called
   * with the locally owned subdomain id.
   *
   * @deprecated Use the previous function instead.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  extract_locally_active_dofs(const DoFHandler<dim, spacedim> &dof_handler,
                              IndexSet                        &dof_set);

  /**
   * Same function as above but for a certain (multigrid-)level.
   * This function returns all DoF indices that live on
   * all locally owned cells (including on the interface to ghost cells) on the
   * given level.
   */
  template <int dim, int spacedim>
  IndexSet
  extract_locally_active_level_dofs(
    const DoFHandler<dim, spacedim> &dof_handler,
    const unsigned int               level);

  /**
   * Same function as above but for a certain (multigrid-)level.
   * This function returns all DoF indices that live on
   * all locally owned cells (including on the interface to ghost cells) on the
   * given level.
   *
   * @deprecated Use the previous function instead.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  extract_locally_active_level_dofs(
    const DoFHandler<dim, spacedim> &dof_handler,
    IndexSet                        &dof_set,
    const unsigned int               level);

  /**
   * Extract the set of global DoF indices that are active on the current
   * DoFHandler. For regular DoFHandlers, these are all DoF indices, but for
   * DoFHandler objects built on parallel::distributed::Triangulation this set
   * is the union of DoFHandler::locally_owned_dofs() and the DoF indices on
   * all ghost cells. In essence, it is the DoF indices on all cells that are
   * not artificial (see
   * @ref GlossArtificialCell "the glossary").
   */
  template <int dim, int spacedim>
  IndexSet
  extract_locally_relevant_dofs(const DoFHandler<dim, spacedim> &dof_handler);

  /**
   * Extract the set of global DoF indices that are active on the current
   * DoFHandler. For regular DoFHandlers, these are all DoF indices, but for
   * DoFHandler objects built on parallel::distributed::Triangulation this set
   * is the union of DoFHandler::locally_owned_dofs() and the DoF indices on
   * all ghost cells. In essence, it is the DoF indices on all cells that are
   * not artificial (see @ref GlossArtificialCell "the glossary").
   *
   * @deprecated Use the previous function instead.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  extract_locally_relevant_dofs(const DoFHandler<dim, spacedim> &dof_handler,
                                IndexSet                        &dof_set);

  /**
   * Extract the set of locally owned DoF indices for each component within the
   * mask that are owned by the current processor. For components disabled by
   * the mask, an empty IndexSet is returned. For a scalar DoFHandler built on a
   * sequential triangulation, the return vector contains a single complete
   * IndexSet with all DoF indices. If the mask contains all components (which
   * also corresponds to the default value), then the union of the returned
   * index sets equals what DoFHandler::locally_owned_dofs() returns.
   */
  template <int dim, int spacedim>
  std::vector<IndexSet>
  locally_owned_dofs_per_component(const DoFHandler<dim, spacedim> &dof_handler,
                                   const ComponentMask &components = {});

  /**
   * For each processor, determine the set of locally owned degrees of freedom
   * as an IndexSet. This function then returns a vector of index sets, where
   * the vector has size equal to the number of MPI processes that participate
   * in the DoF handler object.
   *
   * The function can be used for objects of type dealii::Triangulation or
   * parallel::shared::Triangulation. It will not work for objects of type
   * parallel::distributed::Triangulation since for such triangulations we do
   * not have information about all cells of the triangulation available
   * locally, and consequently can not say anything definitive about the
   * degrees of freedom active on other processors' locally owned cells.
   */
  template <int dim, int spacedim>
  std::vector<IndexSet>
  locally_owned_dofs_per_subdomain(
    const DoFHandler<dim, spacedim> &dof_handler);

  /**
   *
   * For each processor, determine the set of locally relevant degrees of
   * freedom as an IndexSet. This function then returns a vector of index
   * sets, where the vector has size equal to the number of MPI processes that
   * participate in the DoF handler object.
   *
   * The function can be used for objects of type dealii::Triangulation or
   * parallel::shared::Triangulation. It will not work for objects of type
   * parallel::distributed::Triangulation since for such triangulations we do
   * not have information about all cells of the triangulation available
   * locally, and consequently can not say anything definitive about the
   * degrees of freedom active on other processors' locally owned cells.
   */
  template <int dim, int spacedim>
  std::vector<IndexSet>
  locally_relevant_dofs_per_subdomain(
    const DoFHandler<dim, spacedim> &dof_handler);

  /**
   * Same as extract_locally_relevant_dofs() but for multigrid DoFs for the
   * given @p level.
   */
  template <int dim, int spacedim>
  IndexSet
  extract_locally_relevant_level_dofs(
    const DoFHandler<dim, spacedim> &dof_handler,
    const unsigned int               level);

  /**
   * Same as extract_locally_relevant_dofs() but for multigrid DoFs for the
   * given @p level.
   *
   * @deprecated Use the previous function instead.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  extract_locally_relevant_level_dofs(
    const DoFHandler<dim, spacedim> &dof_handler,
    const unsigned int               level,
    IndexSet                        &dof_set);


  /**
   * For each degree of freedom, return in the output array to which subdomain
   * (as given by the <tt>cell->subdomain_id()</tt> function) it belongs. The
   * output array is supposed to have the right size already when calling this
   * function.
   *
   * Note that degrees of freedom associated with faces, edges, and vertices
   * may be associated with multiple subdomains if they are sitting on
   * partition boundaries. In these cases, we assign them to the process with
   * the smaller subdomain id. This may lead to different numbers of degrees
   * of freedom in partitions, even if the number of cells is perfectly
   * equidistributed. While this is regrettable, it is not a problem in
   * practice since the number of degrees of freedom on partition boundaries
   * is asymptotically vanishing as we refine the mesh as long as the number
   * of partitions is kept constant.
   *
   * This function returns the association of each DoF with one subdomain. If
   * you are looking for the association of each @em cell with a subdomain,
   * either query the <tt>cell->subdomain_id()</tt> function, or use the
   * <tt>GridTools::get_subdomain_association</tt> function.
   *
   * Note that this function is of questionable use for DoFHandler objects
   * built on parallel::distributed::Triangulation since in that case
   * ownership of individual degrees of freedom by MPI processes is controlled
   * by the DoF handler object, not based on some geometric algorithm in
   * conjunction with subdomain id. In particular, the degrees of freedom
   * identified by the functions in this namespace as associated with a
   * subdomain are not the same the DoFHandler class identifies as those it
   * owns.
   */
  template <int dim, int spacedim>
  void
  get_subdomain_association(const DoFHandler<dim, spacedim>  &dof_handler,
                            std::vector<types::subdomain_id> &subdomain);

  /**
   * Count how many degrees of freedom are uniquely associated with the given
   * @p subdomain index.
   *
   * Note that there may be rare cases where cells with the given @p subdomain
   * index exist, but none of its degrees of freedom are actually associated
   * with it. In that case, the returned value will be zero.
   *
   * This function will generate an exception if there are no cells with the
   * given @p subdomain index.
   *
   * This function returns the number of DoFs associated with one subdomain.
   * If you are looking for the association of @em cells with this subdomain,
   * use the <tt>GridTools::count_cells_with_subdomain_association</tt>
   * function.
   *
   * Note that this function is of questionable use for DoFHandler objects
   * built on parallel::distributed::Triangulation since in that case
   * ownership of individual degrees of freedom by MPI processes is controlled
   * by the DoF handler object, not based on some geometric algorithm in
   * conjunction with subdomain id. In particular, the degrees of freedom
   * identified by the functions in this namespace as associated with a
   * subdomain are not the same the DoFHandler class identifies as those it
   * owns.
   */
  template <int dim, int spacedim>
  unsigned int
  count_dofs_with_subdomain_association(
    const DoFHandler<dim, spacedim> &dof_handler,
    const types::subdomain_id        subdomain);

  /**
   * Count how many degrees of freedom are uniquely associated with the given
   * @p subdomain index.
   *
   * This function does what the previous one does except that it splits the
   * result among the vector components of the finite element in use by the
   * DoFHandler object. The last argument (which must have a length equal to
   * the number of vector components) will therefore store how many degrees of
   * freedom of each vector component are associated with the given subdomain.
   *
   * Note that this function is of questionable use for DoFHandler objects
   * built on parallel::distributed::Triangulation since in that case
   * ownership of individual degrees of freedom by MPI processes is controlled
   * by the DoF handler object, not based on some geometric algorithm in
   * conjunction with subdomain id. In particular, the degrees of freedom
   * identified by the functions in this namespace as associated with a
   * subdomain are not the same the DoFHandler class identifies as those it
   * owns.
   */
  template <int dim, int spacedim>
  void
  count_dofs_with_subdomain_association(
    const DoFHandler<dim, spacedim> &dof_handler,
    const types::subdomain_id        subdomain,
    std::vector<unsigned int>       &n_dofs_on_subdomain);

  /**
   * Return a set of indices that denotes the degrees of freedom that live on
   * the given subdomain, i.e. that are on cells owned by the current
   * processor. Note that this includes the ones that this subdomain "owns"
   * (i.e. the ones for which get_subdomain_association() returns a value
   * equal to the subdomain given here and that are selected by the
   * DoFHandler::locally_owned_dofs() function) but also all of those that sit
   * on the boundary between the given subdomain and other subdomain. In
   * essence, degrees of freedom that sit on boundaries between subdomain will
   * be in the index sets returned by this function for more than one
   * subdomain.
   *
   * Note that this function is of questionable use for DoFHandler objects
   * built on parallel::distributed::Triangulation since in that case
   * ownership of individual degrees of freedom by MPI processes is controlled
   * by the DoF handler object, not based on some geometric algorithm in
   * conjunction with subdomain id. In particular, the degrees of freedom
   * identified by the functions in this namespace as associated with a
   * subdomain are not the same the DoFHandler class identifies as those it
   * owns.
   */
  template <int dim, int spacedim>
  IndexSet
  dof_indices_with_subdomain_association(
    const DoFHandler<dim, spacedim> &dof_handler,
    const types::subdomain_id        subdomain);
  /** @} */

  /**
   * @name DoF indices on patches of cells
   *
   * Create structures containing a large set of degrees of freedom for small
   * patches of cells. The resulting objects can be used in RelaxationBlockSOR
   * and related classes to implement Schwarz preconditioners and smoothers,
   * where the subdomains consist of small numbers of cells only.
   */
  /** @{ */

  /**
   * Return the set of degrees of freedom that live on a set of cells (i.e., a
   * patch) described by the argument.
   *
   * Patches are often used in defining error estimators that require the
   * solution of a local problem on the patch surrounding each of the cells of
   * the mesh. You can get a list of cells that form the patch around a given
   * cell using GridTools::get_patch_around_cell(). While
   * DoFTools::count_dofs_on_patch() can be used to determine the size of
   * these local problems, so that one can assemble the local system and then
   * solve it, it is still necessary to provide a mapping between the global
   * indices of the degrees of freedom that live on the patch and a local
   * enumeration. This function provides such a local enumeration by returning
   * the set of degrees of freedom that live on the patch.
   *
   * Since this set is returned in the form of a std::vector, one can also
   * think of it as a mapping
   * @code
   *   i -> global_dof_index
   * @endcode
   * where <code>i</code> is an index into the returned vector (i.e., a the
   * <i>local</i> index of a degree of freedom on the patch) and
   * <code>global_dof_index</code> is the global index of a degree of freedom
   * located on the patch. The array returned has size equal to
   * DoFTools::count_dofs_on_patch().
   *
   * @note The array returned is sorted by global DoF index. Consequently, if
   * one considers the index into this array a local DoF index, then the local
   * system that results retains the block structure of the global system.
   *
   * @param patch A collection of cells within an object of type
   * DoFHandler<dim, spacedim>::active_cell_iterator
   *
   * @return A list of those global degrees of freedom located on the patch,
   * as defined above.
   *
   * @note In the context of a parallel distributed computation, it only makes
   * sense to call this function on patches around locally owned cells. This
   * is because the neighbors of locally owned cells are either locally owned
   * themselves, or ghost cells. For both, we know that these are in fact the
   * real cells of the complete, parallel triangulation. We can also query the
   * degrees of freedom on these. In other words, this function can only work
   * if all cells in the patch are either locally owned or ghost cells.
   */
  template <int dim, int spacedim>
  std::vector<types::global_dof_index>
  get_dofs_on_patch(
    const std::vector<typename DoFHandler<dim, spacedim>::active_cell_iterator>
      &patch);

  /**
   * Creates a sparsity pattern, which lists
   * the degrees of freedom associated to each cell on the given
   * level. This pattern can be used in RelaxationBlock classes as
   * block list for additive and multiplicative Schwarz methods.
   *
   * The row index in this pattern is the cell index resulting from
   * standard iteration through a level of the Triangulation. For a
   * parallel::distributed::Triangulation, only locally owned cells
   * are entered.
   *
   * The sparsity pattern is resized in this function to contain as
   * many rows as there are locally owned cells on a given level, as
   * many columns as there are degrees of freedom on this level.
   *
   * <tt>selected_dofs</tt> is a vector indexed by the local degrees
   * of freedom on a cell. If it is used, only such dofs are entered
   * into the block list which are selected. This allows for instance
   * the exclusion of components or of dofs on the boundary.
   */
  template <int dim, int spacedim>
  void
  make_cell_patches(SparsityPattern                 &block_list,
                    const DoFHandler<dim, spacedim> &dof_handler,
                    const unsigned int               level,
                    const std::vector<bool>         &selected_dofs = {},
                    const types::global_dof_index    offset        = 0);

  /**
   * Create an incidence matrix that for every vertex on a given level of a
   * multilevel DoFHandler flags which degrees of freedom are associated with
   * the adjacent cells. This data structure is a matrix with as many rows as
   * there are vertices on a given level, as many columns as there are degrees
   * of freedom on this level, and entries that are either true or false. This
   * data structure is conveniently represented by a SparsityPattern object.
   * The sparsity pattern may be empty when entering this function and will be
   * reinitialized to the correct size.
   *
   * The function has some boolean arguments (listed below) controlling
   * details of the generated patches. The default settings are those for
   * Arnold-Falk-Winther type smoothers for divergence and curl conforming
   * finite elements with essential boundary conditions. Other applications
   * are possible, in particular changing <tt>boundary_patches</tt> for
   * non-essential boundary conditions.
   *
   * This function returns the <tt>vertex_mapping</tt>,
   * that contains the mapping from the vertex indices to the block indices
   * of the <tt>block_list</tt>. For vertices that do not lead to a vertex
   * patch, the entry in <tt>vertex_mapping</tt> contains the value
   * <tt>invalid_unsigned_int</tt>. If <tt>invert_vertex_mapping</tt> is set to
   * <tt>true</tt>, then the <tt>vertex_mapping</tt> is inverted such that it
   * contains the mapping from the block indices to the corresponding vertex
   * indices.
   *
   * @arg <tt>block_list</tt>: the SparsityPattern into which the patches will
   * be stored.
   *
   * @arg <tt>dof_handler</tt>: the multilevel dof handler providing the
   * topology operated on.
   *
   * @arg <tt>interior_dofs_only</tt>: for each patch of cells around a
   * vertex, collect only the interior degrees of freedom of the patch and
   * disregard those on the boundary of the patch. This is for instance the
   * setting for smoothers of Arnold-Falk-Winther type.
   *
   * @arg <tt>boundary_patches</tt>: include patches around vertices at the
   * boundary of the domain. If not, only patches around interior vertices
   * will be generated.
   *
   * @arg <tt>level_boundary_patches</tt>: same for refinement edges towards
   * coarser cells.
   *
   * @arg <tt>single_cell_patches</tt>: if not true, patches containing a
   * single cell are eliminated.
   *
   * @arg <tt>invert_vertex_mapping</tt>: if true, then the return value
   * contains one vertex index for each block; if false, then the return value
   * contains one block index or <tt>invalid_unsigned_int</tt> for each vertex.
   */
  template <int dim, int spacedim>
  std::vector<unsigned int>
  make_vertex_patches(SparsityPattern                 &block_list,
                      const DoFHandler<dim, spacedim> &dof_handler,
                      const unsigned int               level,
                      const bool                       interior_dofs_only,
                      const bool                       boundary_patches = false,
                      const bool level_boundary_patches                 = false,
                      const bool single_cell_patches                    = false,
                      const bool invert_vertex_mapping = false);

  /**
   * Same as above but allows boundary dofs on blocks to be excluded
   * individually.
   *
   * This is helpful if you want to use, for example, Taylor Hood elements as
   * it allows you to not include the boundary DoFs for the velocity block on
   * the patches while also letting you include the boundary DoFs for the
   * pressure block.
   *
   * For each patch of cells around a vertex, collect all of the interior
   * degrees of freedom of the patch and disregard those on the boundary of
   * the patch if the boolean value for the corresponding block in the
   * BlockMask of @p exclude_boundary_dofs is false.
   */
  template <int dim, int spacedim>
  std::vector<unsigned int>
  make_vertex_patches(SparsityPattern                 &block_list,
                      const DoFHandler<dim, spacedim> &dof_handler,
                      const unsigned int               level,
                      const BlockMask &exclude_boundary_dofs  = BlockMask(),
                      const bool       boundary_patches       = false,
                      const bool       level_boundary_patches = false,
                      const bool       single_cell_patches    = false,
                      const bool       invert_vertex_mapping  = false);

  /**
   * Create an incidence matrix that for every cell on a given level of a
   * multilevel DoFHandler flags which degrees of freedom are associated with
   * children of this cell. This data structure is conveniently represented by
   * a SparsityPattern object.
   *
   * The function thus creates a sparsity pattern which in each row (with rows
   * corresponding to the cells on this level) lists the degrees of freedom
   * associated to the cells that are the children of this cell. The DoF
   * indices used here are level dof indices of a multilevel hierarchy, i.e.,
   * they may be associated with children that are not themselves active. The
   * sparsity pattern may be empty when entering this function and will be
   * reinitialized to the correct size.
   *
   * The function has some boolean arguments (listed below) controlling
   * details of the generated patches. The default settings are those for
   * Arnold-Falk-Winther type smoothers for divergence and curl conforming
   * finite elements with essential boundary conditions. Other applications
   * are possible, in particular changing <tt>boundary_dofs</tt> for
   * non-essential boundary conditions.
   *
   * @arg <tt>block_list</tt>: the SparsityPattern into which the patches will
   * be stored.
   *
   * @arg <tt>dof_handler</tt>: The multilevel dof handler providing the
   * topology operated on.
   *
   * @arg <tt>interior_dofs_only</tt>: for each patch of cells around a
   * vertex, collect only the interior degrees of freedom of the patch and
   * disregard those on the boundary of the patch. This is for instance the
   * setting for smoothers of Arnold-Falk-Winther type.
   *
   * @arg <tt>boundary_dofs</tt>: include degrees of freedom, which would have
   * excluded by <tt>interior_dofs_only</tt>, but are lying on the boundary of
   * the domain, and thus need smoothing. This parameter has no effect if
   * <tt>interior_dofs_only</tt> is false.
   */
  template <int dim, int spacedim>
  void
  make_child_patches(SparsityPattern                 &block_list,
                     const DoFHandler<dim, spacedim> &dof_handler,
                     const unsigned int               level,
                     const bool                       interior_dofs_only,
                     const bool                       boundary_dofs = false);

  /**
   * Create a block list with only a single patch, which in turn contains all
   * degrees of freedom on the given level.
   *
   * This function is mostly a closure on level 0 for functions like
   * make_child_patches() and make_vertex_patches(), which may produce an
   * empty patch list.
   *
   * @arg <tt>block_list</tt>: the SparsityPattern into which the patches will
   * be stored.
   *
   * @arg <tt>dof_handler</tt>: The multilevel dof handler providing the
   * topology operated on.
   *
   * @arg <tt>level</tt> The grid level used for building the list.
   *
   * @arg <tt>interior_dofs_only</tt>: if true, exclude degrees of freedom on
   * the boundary of the domain.
   */
  template <int dim, int spacedim>
  void
  make_single_patch(SparsityPattern                 &block_list,
                    const DoFHandler<dim, spacedim> &dof_handler,
                    const unsigned int               level,
                    const bool interior_dofs_only = false);

  /**
   * @}
   */
  /**
   * @name Counting degrees of freedom and related functions
   * @{
   */

  /**
   * Count how many degrees of freedom out of the total number belong to each
   * component. If the number of components the finite element has is one
   * (i.e. you only have one scalar variable), then the number in this
   * component obviously equals the total number of degrees of freedom.
   * Otherwise, the sum of the DoFs in all the components needs to equal the
   * total number.
   *
   * However, the last statement does not hold true if the finite element is
   * not primitive, i.e. some or all of its shape functions are non-zero in
   * more than one vector component. This applies, for example, to the Nedelec
   * or Raviart-Thomas elements. In this case, a degree of freedom is counted
   * in each component in which it is non-zero, so that the sum mentioned
   * above is greater than the total number of degrees of freedom.
   *
   * This behavior can be switched off by the optional parameter
   * <tt>vector_valued_once</tt>. If this is <tt>true</tt>, the number of
   * components of a nonprimitive vector valued element is collected only in
   * the first component. All other components will have a count of zero.
   *
   * The additional optional argument @p target_component allows for a
   * re-sorting and grouping of components. To this end, it contains for each
   * component the component number it shall be counted as. Having the same
   * number entered several times sums up several components as the same. One
   * of the applications of this argument is when you want to form block
   * matrices and vectors, but want to pack several components into the same
   * block (for example, when you have @p dim velocities and one pressure, to
   * put all velocities into one block, and the pressure into another).
   *
   * The result is returned in @p dofs_per_component. Note that the size of @p
   * dofs_per_component needs to be enough to hold all the indices specified
   * in @p target_component. If this is not the case, an assertion is thrown.
   * The indices not targeted by target_components are left untouched.
   */
  template <int dim, int spacedim>
  std::vector<types::global_dof_index>
  count_dofs_per_fe_component(
    const DoFHandler<dim, spacedim> &dof_handler,
    const bool                       vector_valued_once = false,
    const std::vector<unsigned int> &target_component   = {});

  /**
   * Count the degrees of freedom in each block. This function is similar to
   * count_dofs_per_component(), with the difference that the counting is done
   * by blocks. See
   * @ref GlossBlock "blocks"
   * in the glossary for details. Again the vectors are assumed to have the
   * correct size before calling this function. If this is not the case, an
   * assertion is thrown.
   *
   * This function is used in the step-22, step-31, and step-32 tutorial
   * programs, among others.
   *
   * @pre The dofs_per_block variable has as many components as the finite
   * element used by the dof_handler argument has blocks, or alternatively as
   * many blocks as are enumerated in the target_blocks argument if given.
   */
  template <int dim, int spacedim>
  std::vector<types::global_dof_index>
  count_dofs_per_fe_block(const DoFHandler<dim, spacedim> &dof,
                          const std::vector<unsigned int> &target_block =
                            std::vector<unsigned int>());

  /**
   * For each active cell of a DoFHandler, extract the active finite element
   * index and fill the vector given as second argument. This vector is assumed
   * to have as many entries as there are active cells.
   *
   * For DoFHandler objects without hp-capabilities given as first argument, the
   * returned vector will consist of only zeros, indicating that all cells use
   * the same finite element. In hp-mode, the values may be different, though.
   *
   * As we do not know the active FE index on artificial cells, we set them to
   * the invalid value numbers::invalid_fe_index.
   *
   * @deprecated Use DoFHandler::get_active_fe_indices() that returns the result
   * vector.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  get_active_fe_indices(const DoFHandler<dim, spacedim> &dof_handler,
                        std::vector<unsigned int>       &active_fe_indices);

  /**
   * Count how many degrees of freedom live on a set of cells (i.e., a patch)
   * described by the argument.
   *
   * Patches are often used in defining error estimators that require the
   * solution of a local problem on the patch surrounding each of the cells of
   * the mesh. You can get a list of cells that form the patch around a given
   * cell using GridTools::get_patch_around_cell(). This function is then
   * useful in setting up the size of the linear system used to solve the
   * local problem on the patch around a cell. The function
   * DoFTools::get_dofs_on_patch() will then help to make the connection
   * between global degrees of freedom and the local ones.
   *
   * @param patch A collection of cells within an object of type
   * DoFHandler<dim, spacedim>
   *
   * @return The number of degrees of freedom associated with the cells of
   * this patch.
   *
   * @note In the context of a parallel distributed computation, it only makes
   * sense to call this function on patches around locally owned cells. This
   * is because the neighbors of locally owned cells are either locally owned
   * themselves, or ghost cells. For both, we know that these are in fact the
   * real cells of the complete, parallel triangulation. We can also query the
   * degrees of freedom on these. In other words, this function can only work
   * if all cells in the patch are either locally owned or ghost cells.
   */
  template <int dim, int spacedim>
  unsigned int
  count_dofs_on_patch(
    const std::vector<typename DoFHandler<dim, spacedim>::active_cell_iterator>
      &patch);

  /**
   * @}
   */

  /**
   * @name Functions that return different DoF mappings
   * @{
   */

  /**
   * Create a mapping from degree of freedom indices to the index of that
   * degree of freedom on the boundary. After this operation,
   * <tt>mapping[dof]</tt> gives the index of the degree of freedom with
   * global number @p dof in the list of degrees of freedom on the boundary.
   * If the degree of freedom requested is not on the boundary, the value of
   * <tt>mapping[dof]</tt> is numbers::invalid_dof_index. This function is
   * mainly used when setting up matrices and vectors on the boundary from the
   * trial functions, which have global numbers, while the matrices and vectors
   * use numbers of the trial functions local to the boundary.
   *
   * Prior content of @p mapping is deleted.
   */
  template <int dim, int spacedim>
  void
  map_dof_to_boundary_indices(const DoFHandler<dim, spacedim>      &dof_handler,
                              std::vector<types::global_dof_index> &mapping);

  /**
   * Same as the previous function, except that only those parts of the
   * boundary are considered for which the boundary indicator is listed in the
   * second argument.
   *
   * See the general doc of this class for more information.
   *
   * @see
   * @ref GlossBoundaryIndicator "Glossary entry on boundary indicators"
   */
  template <int dim, int spacedim>
  void
  map_dof_to_boundary_indices(const DoFHandler<dim, spacedim>    &dof_handler,
                              const std::set<types::boundary_id> &boundary_ids,
                              std::vector<types::global_dof_index> &mapping);

  /**
   * Return a list of support points (see this
   * @ref GlossSupport "glossary entry")
   * for all the degrees of freedom handled by this DoF handler object. This
   * function, of course, only works if the finite element object used by the
   * DoF handler object actually provides support points; this rules out
   * "modal" elements in which the shape functions are not defined via point
   * evaluation at individual node points, but by integrals. In practice, this
   * function checks the requirement by requiring that the element in question
   * returns `true` from the FiniteElement::has_support_points().
   *
   * @pre The given array `support_points` must have a length of as many
   * elements as there are degrees of freedom.
   *
   * @note The precondition to this function that the output argument needs to
   * have size equal to the total number of degrees of freedom makes this
   * function unsuitable for the case that the given DoFHandler object derives
   * from a parallel::TriangulationBase object (or any of the classes derived
   * from parallel::TriangulationBase). Consequently, this function will produce
   * an error if called with such a DoFHandler.
   *
   * @param[in] mapping The mapping from the reference cell to the real cell on
   * which DoFs are defined.
   * @param[in] dof_handler The object that describes which DoF indices live on
   * which cell of the triangulation.
   * @param[in,out] support_points A vector that stores the corresponding
   * location of the dofs in real space coordinates. Previous content of this
   * object is deleted in this function.
   * @param[in] mask An optional component mask that restricts the
   * components from which the support points are extracted.
   */
  template <int dim, int spacedim>
  void
  map_dofs_to_support_points(const Mapping<dim, spacedim>    &mapping,
                             const DoFHandler<dim, spacedim> &dof_handler,
                             std::vector<Point<spacedim>>    &support_points,
                             const ComponentMask             &mask = {});

  /**
   * Same as the previous function but for the hp-case.
   */
  template <int dim, int spacedim>
  void
  map_dofs_to_support_points(
    const hp::MappingCollection<dim, spacedim> &mapping,
    const DoFHandler<dim, spacedim>            &dof_handler,
    std::vector<Point<spacedim>>               &support_points,
    const ComponentMask                        &mask = {});

  /**
   * This function is a version of the above map_dofs_to_support_points
   * function that doesn't simply return a vector of support points (see this
   * @ref GlossSupport "glossary entry")
   * with one entry for each global degree of freedom, but instead a map that
   * maps from the DoFs index to its location. The point of this function is
   * that it is also usable in cases where the DoFHandler is based on a
   * parallel::TriangulationBase object (or any of the classes derived from
   * parallel::TriangulationBase). In such cases, each
   * processor will not be able to determine the support point location of all
   * DoFs, and worse no processor may be able to hold a vector that would
   * contain the locations of all DoFs even if they were known. As a
   * consequence, this function constructs a map from those DoFs for which we
   * can know the locations (namely, those DoFs that are locally relevant (see
   * @ref GlossLocallyRelevantDof "locally relevant DoFs")
   * to their locations.
   *
   * For non-distributed triangulations, the map returned as @p support_points
   * is of course dense, i.e., every DoF is to be found in it.
   *
   * @param[in] mapping The mapping from the reference cell to the real cell on
   *   which DoFs are defined.
   * @param[in] dof_handler The object that describes which DoF indices live on
   *   which cell of the triangulation.
   * @param[in] mask An optional component mask that restricts the
   *   components from which the support points are extracted.
   * @return A map that for every locally relevant DoF
   *   index contains the corresponding location in real space coordinates.
   */
  template <int dim, int spacedim>
  std::map<types::global_dof_index, Point<spacedim>>
  map_dofs_to_support_points(const Mapping<dim, spacedim>    &mapping,
                             const DoFHandler<dim, spacedim> &dof_handler,
                             const ComponentMask             &mask = {});

  /**
   * Same as the previous function but for the hp-case.
   */
  template <int dim, int spacedim>
  std::map<types::global_dof_index, Point<spacedim>>
  map_dofs_to_support_points(
    const hp::MappingCollection<dim, spacedim> &mapping,
    const DoFHandler<dim, spacedim>            &dof_handler,
    const ComponentMask                        &mask = {});

  /**
   * A version of the function of same name that returns the map via its third
   * argument. This function is deprecated.
   * @deprecated Use the function that returns the `std::map` instead.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  map_dofs_to_support_points(
    const Mapping<dim, spacedim>                       &mapping,
    const DoFHandler<dim, spacedim>                    &dof_handler,
    std::map<types::global_dof_index, Point<spacedim>> &support_points,
    const ComponentMask                                &mask = {});

  /**
   * A version of the function of same name that returns the map via its third
   * argument. This function is deprecated.
   * @deprecated Use the function that returns the `std::map` instead.
   */
  template <int dim, int spacedim>
  DEAL_II_DEPRECATED void
  map_dofs_to_support_points(
    const hp::MappingCollection<dim, spacedim>         &mapping,
    const DoFHandler<dim, spacedim>                    &dof_handler,
    std::map<types::global_dof_index, Point<spacedim>> &support_points,
    const ComponentMask                                &mask = {});


  /**
   * This is the opposite function to the one above. It generates a map where
   * the keys are the support points of the degrees of freedom, while the
   * values are the DoF indices. For a definition of support points, see this
   * @ref GlossSupport "glossary entry".
   *
   * Since there is no natural order in the space of points (except for the 1d
   * case), you have to provide a map with an explicitly specified comparator
   * object. This function is therefore templatized on the comparator object.
   * Previous content of the map object is deleted in this function.
   *
   * Just as with the function above, it is assumed that the finite element in
   * use here actually supports the notion of support points of all its
   * components.
   *
   * @todo This function should generate a multimap, rather than just a map,
   * since several dofs may be located at the same support point. Currently,
   * only the last value in the map returned by map_dofs_to_support_points() for
   * each point will be returned.
   */
  template <int dim, int spacedim, class Comp>
  void
  map_support_points_to_dofs(
    const Mapping<dim, spacedim>    &mapping,
    const DoFHandler<dim, spacedim> &dof_handler,
    std::map<Point<spacedim>, types::global_dof_index, Comp>
      &point_to_index_map);
  /**
   * @}
   */

  /**
   * @name Miscellaneous
   * @{
   */

  /**
   * Take a vector of values which live on cells (e.g. an error per cell) and
   * distribute it to the dofs in such a way that a finite element field
   * results, which can then be further processed, e.g. for output. You should
   * note that the resulting field will not be continuous at hanging nodes.
   * This can, however, easily be arranged by calling the appropriate @p
   * distribute function of an AffineConstraints object created for this
   * DoFHandler object, after the vector has been fully assembled.
   *
   * It is assumed that the number of elements in @p cell_data equals the
   * number of active cells and that the number of elements in @p dof_data
   * equals <tt>dof_handler.n_dofs()</tt>.
   *
   * Note that the input vector may be a vector of any data type as long as it
   * is convertible to @p double.  The output vector, being a data vector on a
   * DoF handler, always consists of elements of type @p double.
   *
   * In case the finite element used by this DoFHandler consists of more than
   * one component, you need to specify which component in the output vector
   * should be used to store the finite element field in; the default is zero
   * (no other value is allowed if the finite element consists only of one
   * component). All other components of the vector remain untouched, i.e.
   * their contents are not changed.
   *
   * This function cannot be used if the finite element in use has shape
   * functions that are non-zero in more than one vector component (in deal.II
   * speak: they are non-primitive).
   */
  template <int dim, int spacedim, typename Number>
  void
  distribute_cell_to_dof_vector(const DoFHandler<dim, spacedim> &dof_handler,
                                const Vector<Number>            &cell_data,
                                Vector<double>                  &dof_data,
                                const unsigned int               component = 0);


  /**
   * Generate text output readable by gnuplot with point data based on the
   * given map @p support_points.  For each support point location, a string
   * label containing a list of all DoFs from the map is generated.  The map
   * can be generated with a call to map_dofs_to_support_points() and is useful
   * to visualize location and global numbering of unknowns. This function is
   * used in step-2.
   *
   * An example for the format of each line in the output is:
   * @code
   * x [y] [z] "dof1, dof2"
   * @endcode
   * where x, y, and z (present only in corresponding dimension) are the
   * coordinates of the support point, followed by a list of DoF numbers.
   *
   * The points with labels can be plotted as follows in gnuplot:
   * @code
   * plot "./points.gpl" using 1:2:3 with labels point offset 1,1
   * @endcode
   *
   * Examples (this also includes the grid written separately using GridOut):
   * <p ALIGN="center">
   * @image html support_point_dofs1.png
   * @image html support_point_dofs2.png
   * </p>
   *
   * To generate the mesh and the support point information in a
   * single gnuplot file, use code similar to
   * @code
   * std::ofstream out("gnuplot.gpl");
   * out << "plot '-' using 1:2 with lines, "
   *     << "'-' with labels point pt 2 offset 1,1"
   *     << std::endl;
   * GridOut().write_gnuplot (triangulation, out);
   * out << 'e' << std::endl;
   *
   * std::map<types::global_dof_index, Point<dim> > support_points;
   * DoFTools::map_dofs_to_support_points (MappingQ1<dim>(),
   *                                       dof_handler,
   *                                       support_points);
   * DoFTools::write_gnuplot_dof_support_point_info(out,
   *                                                support_points);
   * out << 'e' << std::endl;
   * @endcode
   * and from within gnuplot execute the following command:
   * @code
   * load "gnuplot.gpl"
   * @endcode
   *
   * Alternatively, the following gnuplot script will generate a png file when
   * executed as <tt>gnuplot gnuplot.gpl</tt> on the command line:
   * @code
   * std::ofstream out("gnuplot.gpl");
   *
   * out << "set terminal png size 400,410 enhanced font \"Helvetica,8\"\n"
   *     << "set output \"output.png\"\n"
   *     << "set size square\n"
   *     << "set view equal xy\n"
   *     << "unset xtics\n"
   *     << "unset ytics\n"
   *     << "unset grid\n"
   *     << "unset border\n"
   *     << "plot '-' using 1:2 with lines notitle, "
   *     << "'-' with labels point pt 2 offset 1,1 notitle"
   *     << std::endl;
   * GridOut().write_gnuplot (triangulation, out);
   * out << 'e' << std::endl;
   *
   * std::map<types::global_dof_index, Point<dim> > support_points;
   * DoFTools::map_dofs_to_support_points (MappingQ1<dim>(),
   *                                       dof_handler,
   *                                       support_points);
   * DoFTools::write_gnuplot_dof_support_point_info(out,
   *                                                support_points);
   * out << 'e' << std::endl;
   * @endcode
   */
  template <int spacedim>
  void
  write_gnuplot_dof_support_point_info(
    std::ostream                                             &out,
    const std::map<types::global_dof_index, Point<spacedim>> &support_points);


  /**
   * Add constraints to @p zero_boundary_constraints corresponding to
   * enforcing a zero boundary condition on the given boundary indicator.
   *
   * This function constrains all degrees of freedom on the given part of the
   * boundary.
   *
   * A variant of this function with different arguments is used in step-36.
   *
   * @param dof The DoFHandler to work on.
   * @param boundary_id The indicator of that part of the boundary for which
   * constraints should be computed. If this number equals
   * numbers::invalid_boundary_id then all boundaries of the domain will be
   * treated.
   * @param zero_boundary_constraints The constraint object into which the
   * constraints will be written. The new constraints due to zero boundary
   * values will simply be added, preserving any other constraints previously
   * present. However, this will only work if the previous content of that
   * object consists of constraints on degrees of freedom that are not located
   * on the boundary treated here. If there are previously existing
   * constraints for degrees of freedom located on the boundary, then this
   * would constitute a conflict. See the
   * @ref constraints
   * topic for handling the case where there are conflicting constraints on
   * individual degrees of freedom.
   * @param component_mask An optional component mask that restricts the
   * functionality of this function to a subset of an FESystem. For
   * non-@ref GlossPrimitive "primitive"
   * shape functions, any degree of freedom is affected that belongs to a
   * shape function where at least one of its nonzero components is affected
   * by the component mask (see
   * @ref GlossComponentMask).
   * If this argument is omitted, all components of the finite element with
   * degrees of freedom at the boundary will be considered.
   *
   * @ingroup constraints
   *
   * @see
   * @ref GlossBoundaryIndicator "Glossary entry on boundary indicators"
   */
  template <int dim, int spacedim, typename number>
  void
  make_zero_boundary_constraints(
    const DoFHandler<dim, spacedim> &dof,
    const types::boundary_id         boundary_id,
    AffineConstraints<number>       &zero_boundary_constraints,
    const ComponentMask             &component_mask = {});

  /**
   * Do the same as the previous function, except do it for all parts of the
   * boundary, not just those with a particular boundary indicator. This
   * function is then equivalent to calling the previous one with
   * numbers::invalid_boundary_id as second argument.
   *
   * This function is used in step-36, for example.
   *
   * @ingroup constraints
   */
  template <int dim, int spacedim, typename number>
  void
  make_zero_boundary_constraints(
    const DoFHandler<dim, spacedim> &dof,
    AffineConstraints<number>       &zero_boundary_constraints,
    const ComponentMask             &component_mask = {});

  /**
   * @}
   */

  /**
   * @name Exceptions
   * @{
   */

  /**
   * @todo Write description
   *
   * @ingroup Exceptions
   */
  DeclException0(ExcFiniteElementsDontMatch);
  /**
   * @todo Write description
   *
   * @ingroup Exceptions
   */
  DeclException0(ExcGridNotCoarser);
  /**
   * @todo Write description
   *
   * Exception
   * @ingroup Exceptions
   */
  DeclException0(ExcGridsDontMatch);
  /**
   * The DoFHandler was not initialized with a finite element. Please call
   * DoFHandler::distribute_dofs() first.
   *
   * @ingroup Exceptions
   */
  DeclException0(ExcNoFESelected);
  /**
   * @todo Write description
   *
   * @ingroup Exceptions
   */
  DeclException0(ExcInvalidBoundaryIndicator);
  /**
   * @}
   */
} // namespace DoFTools



/* ------------------------- inline functions -------------- */

#ifndef DOXYGEN

namespace DoFTools
{
  /**
   * Operator computing the maximum coupling out of two.
   *
   * @relatesalso DoFTools
   */
  inline Coupling
  operator|=(Coupling &c1, const Coupling c2)
  {
    if (c2 == always)
      c1 = always;
    else if (c1 != always && c2 == nonzero)
      return c1 = nonzero;
    return c1;
  }


  /**
   * Operator computing the maximum coupling out of two.
   *
   * @relatesalso DoFTools
   */
  inline Coupling
  operator|(const Coupling c1, const Coupling c2)
  {
    if (c1 == always || c2 == always)
      return always;
    if (c1 == nonzero || c2 == nonzero)
      return nonzero;
    return none;
  }


  // ---------------------- inline and template functions --------------------

  template <int dim, int spacedim, class Comp>
  void
  map_support_points_to_dofs(
    const Mapping<dim, spacedim>    &mapping,
    const DoFHandler<dim, spacedim> &dof_handler,
    std::map<Point<spacedim>, types::global_dof_index, Comp>
      &point_to_index_map)
  {
    // let the checking of arguments be
    // done by the function first
    // called
    std::vector<Point<spacedim>> support_points(dof_handler.n_dofs());
    map_dofs_to_support_points(mapping, dof_handler, support_points);
    // now copy over the results of the
    // previous function into the
    // output arg
    point_to_index_map.clear();
    for (types::global_dof_index i = 0; i < dof_handler.n_dofs(); ++i)
      point_to_index_map[support_points[i]] = i;
  }
} // namespace DoFTools

#endif

DEAL_II_NAMESPACE_CLOSE

#endif
