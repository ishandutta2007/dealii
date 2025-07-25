/* ------------------------------------------------------------------------
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 * Copyright (C) 2008 - 2025 by the deal.II authors
 *
 * This file is part of the deal.II library.
 *
 * Part of the source code is dual licensed under Apache-2.0 WITH
 * LLVM-exception OR LGPL-2.1-or-later. Detailed license information
 * governing the source code and code contributions can be found in
 * LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
 *
 * ------------------------------------------------------------------------
 *
 * Author: Wolfgang Bangerth, Texas A&M University, 2008
 */


// @sect3{Include files}

// As usual, we start by including some well-known files:
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/function.h>
#include <deal.II/base/utilities.h>
#include <deal.II/base/tensor_function.h>

#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/block_sparse_matrix.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/precondition.h>
#include <deal.II/lac/affine_constraints.h>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/grid/grid_refinement.h>

#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_renumbering.h>
#include <deal.II/dofs/dof_tools.h>


#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/numerics/vector_tools.h>
#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/error_estimator.h>

// Then we need to include the header file for the sparse direct solver
// UMFPACK:
#include <deal.II/lac/sparse_direct.h>

// This includes the library for the incomplete LU factorization that will be
// used as a preconditioner in 3d:
#include <deal.II/lac/sparse_ilu.h>

// This is C++:
#include <iostream>
#include <fstream>
#include <memory>

// As in all programs, the namespace dealii is included:
namespace Step22
{
  using namespace dealii;

  // @sect3{Defining the inner preconditioner type}

  // As explained in the introduction, we are going to use different
  // preconditioners for two and three space dimensions, respectively. We
  // distinguish between them by the use of the spatial dimension as a
  // template parameter. See step-4 for details on templates. We are not going
  // to create any preconditioner object here, all we do is to create class
  // that holds a local alias determining the preconditioner class so we can
  // write our program in a dimension-independent way.
  template <int dim>
  struct InnerPreconditioner;

  // In 2d, we are going to use a sparse direct solver as preconditioner:
  template <>
  struct InnerPreconditioner<2>
  {
    using type = SparseDirectUMFPACK;
  };

  // And the ILU preconditioning in 3d, called by SparseILU:
  template <>
  struct InnerPreconditioner<3>
  {
    using type = SparseILU<double>;
  };


  // @sect3{The <code>StokesProblem</code> class template}

  // This is an adaptation of step-20, so the main class and the data types
  // are nearly the same as used there. The only difference is that we have an
  // additional member <code>preconditioner_matrix</code>, that is used for
  // preconditioning the Schur complement, and a corresponding sparsity
  // pattern <code>preconditioner_sparsity_pattern</code>. In addition,
  // instead of relying on LinearOperator, we implement our own InverseMatrix
  // class.
  //
  // In this example we also use adaptive grid refinement, which is handled
  // in analogy to step-6. According to the discussion in the introduction,
  // we are also going to use the AffineConstraints object for implementing
  // Dirichlet boundary conditions. Hence, we change the name
  // <code>hanging_node_constraints</code> into <code>constraints</code>.
  template <int dim>
  class StokesProblem
  {
  public:
    StokesProblem(const unsigned int degree);
    void run();

  private:
    void setup_dofs();
    void assemble_system();
    void solve();
    void output_results(const unsigned int refinement_cycle) const;
    void refine_mesh();

    const unsigned int degree;

    Triangulation<dim>  triangulation;
    const FESystem<dim> fe;
    DoFHandler<dim>     dof_handler;

    AffineConstraints<double> constraints;

    BlockSparsityPattern      sparsity_pattern;
    BlockSparseMatrix<double> system_matrix;

    BlockSparsityPattern      preconditioner_sparsity_pattern;
    BlockSparseMatrix<double> preconditioner_matrix;

    BlockVector<double> solution;
    BlockVector<double> system_rhs;

    // This one is new: We shall use a so-called shared pointer structure to
    // access the preconditioner. Shared pointers are essentially just a
    // convenient form of pointers. Several shared pointers can point to the
    // same object (just like regular pointers), but when the last shared
    // pointer object to point to a preconditioner object is deleted (for
    // example if a shared pointer object goes out of scope, if the class of
    // which it is a member is destroyed, or if the pointer is assigned a
    // different preconditioner object) then the preconditioner object pointed
    // to is also destroyed. This ensures that we don't have to manually track
    // in how many places a preconditioner object is still referenced, it can
    // never create a memory leak, and can never produce a dangling pointer to
    // an already destroyed object:
    std::shared_ptr<typename InnerPreconditioner<dim>::type> A_preconditioner;
  };

  // @sect3{Boundary values and right hand side}

  // As in step-20 and most other example programs, the next task is to define
  // the data for the PDE: For the Stokes problem, we are going to use natural
  // boundary values on parts of the boundary (i.e. homogeneous Neumann-type)
  // for which we won't have to do anything special (the homogeneity implies
  // that the corresponding terms in the weak form are simply zero), and
  // boundary conditions on the velocity (Dirichlet-type) on the rest of the
  // boundary, as described in the introduction.
  //
  // In order to enforce the Dirichlet boundary values on the velocity, we
  // will use the VectorTools::interpolate_boundary_values function as usual
  // which requires us to write a function object with as many components as
  // the finite element has. In other words, we have to define the function on
  // the $(u,p)$-space, but we are going to filter out the pressure component
  // when interpolating the boundary values.

  // The following function object is a representation of the boundary values
  // described in the introduction:
  template <int dim>
  class BoundaryValues : public Function<dim>
  {
  public:
    BoundaryValues()
      : Function<dim>(dim + 1)
    {}

    virtual double value(const Point<dim>  &p,
                         const unsigned int component = 0) const override;

    virtual void vector_value(const Point<dim> &p,
                              Vector<double>   &value) const override;
  };


  template <int dim>
  double BoundaryValues<dim>::value(const Point<dim>  &p,
                                    const unsigned int component) const
  {
    Assert(component < this->n_components,
           ExcIndexRange(component, 0, this->n_components));

    if (component == 0)
      {
        if (p[0] < 0)
          return -1;
        else if (p[0] > 0)
          return 1;
        else
          return 0;
      }

    return 0;
  }


  template <int dim>
  void BoundaryValues<dim>::vector_value(const Point<dim> &p,
                                         Vector<double>   &values) const
  {
    for (unsigned int c = 0; c < this->n_components; ++c)
      values(c) = BoundaryValues<dim>::value(p, c);
  }



  // We implement similar functions for the right hand side which for the
  // current example is simply zero. In the current case, what we want to
  // represent is the right hand side of the velocity equation, that is, the
  // body forces acting on the fluid. (The right hand side of the divergence
  // equation is always zero, since we are dealing with an incompressible
  // medium.) As such, the right hand side is represented by a `Tensor<1,dim>`
  // object: a dim-dimensional vector, and the right tool to implement such a
  // function is to derive it from `TensorFunction<1,dim>`:
  template <int dim>
  class RightHandSide : public TensorFunction<1, dim>
  {
  public:
    RightHandSide()
      : TensorFunction<1, dim>()
    {}

    virtual Tensor<1, dim> value(const Point<dim> &p) const override;

    virtual void value_list(const std::vector<Point<dim>> &p,
                            std::vector<Tensor<1, dim>> &value) const override;
  };


  template <int dim>
  Tensor<1, dim> RightHandSide<dim>::value(const Point<dim> & /*p*/) const
  {
    return Tensor<1, dim>();
  }


  template <int dim>
  void RightHandSide<dim>::value_list(const std::vector<Point<dim>> &vp,
                                      std::vector<Tensor<1, dim>> &values) const
  {
    for (unsigned int c = 0; c < vp.size(); ++c)
      {
        values[c] = RightHandSide<dim>::value(vp[c]);
      }
  }


  // @sect3{Linear solvers and preconditioners}

  // The linear solvers and preconditioners are discussed extensively in the
  // introduction. Here, we create the respective objects that will be used.

  // @sect4{The <code>InverseMatrix</code> class template}
  // The <code>InverseMatrix</code> class represents the data structure for an
  // inverse matrix. Unlike step-20, we implement this with a class instead of
  // the helper function inverse_linear_operator() we will apply this class to
  // different kinds of matrices that will require different preconditioners
  // (in step-20 we only used a non-identity preconditioner for the mass
  // matrix). The types of matrix and preconditioner are passed to this class
  // via template parameters, and matrix and preconditioner objects of these
  // types will then be passed to the constructor when an
  // <code>InverseMatrix</code> object is created. The member function
  // <code>vmult</code> is obtained by solving a linear system:
  template <class MatrixType, class PreconditionerType>
  class InverseMatrix : public EnableObserverPointer
  {
  public:
    InverseMatrix(const MatrixType         &m,
                  const PreconditionerType &preconditioner);

    void vmult(Vector<double> &dst, const Vector<double> &src) const;

  private:
    const ObserverPointer<const MatrixType>         matrix;
    const ObserverPointer<const PreconditionerType> preconditioner;
  };


  template <class MatrixType, class PreconditionerType>
  InverseMatrix<MatrixType, PreconditionerType>::InverseMatrix(
    const MatrixType         &m,
    const PreconditionerType &preconditioner)
    : matrix(&m)
    , preconditioner(&preconditioner)
  {}


  // This is the implementation of the <code>vmult</code> function.

  // In this class we use a rather large tolerance for the solver control. The
  // reason for this is that the function is used very frequently, and hence,
  // any additional effort to make the residual in the CG solve smaller makes
  // the solution more expensive. Note that we do not only use this class as a
  // preconditioner for the Schur complement, but also when forming the
  // inverse of the Laplace matrix &ndash; which is hence directly responsible
  // for the accuracy of the solution itself, so we can't choose a too large
  // tolerance, either.
  template <class MatrixType, class PreconditionerType>
  void InverseMatrix<MatrixType, PreconditionerType>::vmult(
    Vector<double>       &dst,
    const Vector<double> &src) const
  {
    SolverControl            solver_control(src.size(), 1e-6 * src.l2_norm());
    SolverCG<Vector<double>> cg(solver_control);

    dst = 0;

    cg.solve(*matrix, dst, src, *preconditioner);
  }


  // @sect4{The <code>SchurComplement</code> class template}

  // This class implements the Schur complement discussed in the introduction.
  // It is in analogy to step-20.  Though, we now call it with a template
  // parameter <code>PreconditionerType</code> in order to access that when
  // specifying the respective type of the inverse matrix class. As a
  // consequence of the definition above, the declaration
  // <code>InverseMatrix</code> now contains the second template parameter for
  // a preconditioner class as above, which affects the
  // <code>ObserverPointer</code> object <code>m_inverse</code> as well.
  template <class PreconditionerType>
  class SchurComplement : public EnableObserverPointer
  {
  public:
    SchurComplement(
      const BlockSparseMatrix<double> &system_matrix,
      const InverseMatrix<SparseMatrix<double>, PreconditionerType> &A_inverse);

    void vmult(Vector<double> &dst, const Vector<double> &src) const;

  private:
    const ObserverPointer<const BlockSparseMatrix<double>> system_matrix;
    const ObserverPointer<
      const InverseMatrix<SparseMatrix<double>, PreconditionerType>>
      A_inverse;

    mutable Vector<double> tmp1, tmp2;
  };



  template <class PreconditionerType>
  SchurComplement<PreconditionerType>::SchurComplement(
    const BlockSparseMatrix<double> &system_matrix,
    const InverseMatrix<SparseMatrix<double>, PreconditionerType> &A_inverse)
    : system_matrix(&system_matrix)
    , A_inverse(&A_inverse)
    , tmp1(system_matrix.block(0, 0).m())
    , tmp2(system_matrix.block(0, 0).m())
  {}


  template <class PreconditionerType>
  void
  SchurComplement<PreconditionerType>::vmult(Vector<double>       &dst,
                                             const Vector<double> &src) const
  {
    system_matrix->block(0, 1).vmult(tmp1, src);
    A_inverse->vmult(tmp2, tmp1);
    system_matrix->block(1, 0).vmult(dst, tmp2);
  }


  // @sect3{StokesProblem class implementation}

  // @sect4{StokesProblem::StokesProblem}

  // The constructor of this class looks very similar to the one of
  // step-20. The constructor initializes the variables for the polynomial
  // degree, triangulation, finite element system and the dof handler. The
  // underlying polynomial functions are of order <code>degree+1</code> for
  // the vector-valued velocity components and of order <code>degree</code>
  // for the pressure.  This gives the LBB-stable element pair
  // $Q_{degree+1}^d\times Q_{degree}$, often referred to as the Taylor-Hood
  // element for degree$\geq 1$.
  //
  // Note that we initialize the triangulation with a MeshSmoothing argument,
  // which ensures that the refinement of cells is done in a way that the
  // approximation of the PDE solution remains well-behaved (problems arise if
  // grids are too unstructured), see the documentation of
  // <code>Triangulation::MeshSmoothing</code> for details.
  template <int dim>
  StokesProblem<dim>::StokesProblem(const unsigned int degree)
    : degree(degree)
    , triangulation(Triangulation<dim>::maximum_smoothing)
    , fe(FE_Q<dim>(degree + 1) ^ dim, FE_Q<dim>(degree))
    , dof_handler(triangulation)
  {}


  // @sect4{StokesProblem::setup_dofs}

  // Given a mesh, this function associates the degrees of freedom with it and
  // creates the corresponding matrices and vectors. At the beginning it also
  // releases the pointer to the preconditioner object (if the shared pointer
  // pointed at anything at all at this point) since it will definitely not be
  // needed any more after this point and will have to be re-computed after
  // assembling the matrix, and unties the sparse matrices from their sparsity
  // pattern objects.
  //
  // We then proceed with distributing degrees of freedom and renumbering
  // them: In order to make the ILU preconditioner (in 3d) work efficiently,
  // it is important to enumerate the degrees of freedom in such a way that it
  // reduces the bandwidth of the matrix, or maybe more importantly: in such a
  // way that the ILU is as close as possible to a real LU decomposition. On
  // the other hand, we need to preserve the block structure of velocity and
  // pressure already seen in step-20 and step-21. This is done in two
  // steps: First, all dofs are renumbered to improve the ILU and then we
  // renumber once again by components. Since
  // <code>DoFRenumbering::component_wise</code> does not touch the
  // renumbering within the individual blocks, the basic renumbering from the
  // first step remains. As for how the renumber degrees of freedom to improve
  // the ILU: deal.II has a number of algorithms that attempt to find
  // orderings to improve ILUs, or reduce the bandwidth of matrices, or
  // optimize some other aspect. The DoFRenumbering namespace shows a
  // comparison of the results we obtain with several of these algorithms
  // based on the testcase discussed here in this tutorial program. Here, we
  // will use the traditional Cuthill-McKee algorithm already used in some of
  // the previous tutorial programs.  In the
  // @ref step_22-ImprovedILU "section on improved ILU" we're going to discuss
  // this issue in more detail.

  // There is one more change compared to previous tutorial programs: There is
  // no reason in sorting the <code>dim</code> velocity components
  // individually. In fact, rather than first enumerating all $x$-velocities,
  // then all $y$-velocities, etc, we would like to keep all velocities at the
  // same location together and only separate between velocities (all
  // components) and pressures. By default, this is not what the
  // DoFRenumbering::component_wise function does: it treats each vector
  // component separately; what we have to do is group several components into
  // "blocks" and pass this block structure to that function. Consequently, we
  // allocate a vector <code>block_component</code> with as many elements as
  // there are components and describe all velocity components to correspond
  // to block 0, while the pressure component will form block 1:
  template <int dim>
  void StokesProblem<dim>::setup_dofs()
  {
    A_preconditioner.reset();
    system_matrix.clear();
    preconditioner_matrix.clear();

    dof_handler.distribute_dofs(fe);
    DoFRenumbering::Cuthill_McKee(dof_handler);

    std::vector<unsigned int> block_component(dim + 1, 0);
    block_component[dim] = 1;
    DoFRenumbering::component_wise(dof_handler, block_component);

    // Now comes the implementation of Dirichlet boundary conditions, which
    // should be evident after the discussion in the introduction. All that
    // changed is that the function already appears in the setup functions,
    // whereas we were used to see it in some assembly routine. Further down
    // below where we set up the mesh, we will associate the top boundary
    // where we impose Dirichlet boundary conditions with boundary indicator
    // 1.  We will have to pass this boundary indicator as second argument to
    // the function below interpolating boundary values.  There is one more
    // thing, though.  The function describing the Dirichlet conditions was
    // defined for all components, both velocity and pressure. However, the
    // Dirichlet conditions are to be set for the velocity only.  To this end,
    // we use a ComponentMask that only selects the velocity components. The
    // component mask is obtained from the finite element by specifying the
    // particular components we want. Since we use adaptively refined grids,
    // the affine constraints object needs to be first filled with hanging node
    // constraints generated from the DoF handler. Note the order of the two
    // functions &mdash; we first compute the hanging node constraints, and
    // then insert the boundary values into the constraints object. This makes
    // sure that we respect H<sup>1</sup> conformity on boundaries with
    // hanging nodes (in three space dimensions), where the hanging node needs
    // to dominate the Dirichlet boundary values.
    {
      constraints.clear();

      const FEValuesExtractors::Vector velocities(0);
      DoFTools::make_hanging_node_constraints(dof_handler, constraints);
      VectorTools::interpolate_boundary_values(dof_handler,
                                               1,
                                               BoundaryValues<dim>(),
                                               constraints,
                                               fe.component_mask(velocities));
    }

    constraints.close();

    // In analogy to step-20, we count the dofs in the individual components.
    // We could do this in the same way as there, but we want to operate on
    // the block structure we used already for the renumbering: The function
    // <code>DoFTools::count_dofs_per_fe_block</code> does the same as
    // <code>DoFTools::count_dofs_per_fe_component</code>, but now grouped as
    // velocity and pressure block via <code>block_component</code>.
    const std::vector<types::global_dof_index> dofs_per_block =
      DoFTools::count_dofs_per_fe_block(dof_handler, block_component);
    const types::global_dof_index n_u = dofs_per_block[0];
    const types::global_dof_index n_p = dofs_per_block[1];

    std::cout << "   Number of active cells: " << triangulation.n_active_cells()
              << std::endl
              << "   Number of degrees of freedom: " << dof_handler.n_dofs()
              << " (" << n_u << '+' << n_p << ')' << std::endl;

    // The next task is to allocate a sparsity pattern for the system matrix we
    // will create and one for the preconditioner matrix. We could do this in
    // the same way as in step-20, i.e. directly build an object of type
    // SparsityPattern through DoFTools::make_sparsity_pattern. However, there
    // is a major reason not to do so:
    // In 3d, the function DoFTools::max_couplings_between_dofs yields a
    // conservative but rather large number for the coupling between the
    // individual dofs, so that the memory initially provided for the creation
    // of the sparsity pattern of the matrix is far too much -- so much actually
    // that the initial sparsity pattern won't even fit into the physical memory
    // of most systems already for moderately-sized 3d problems, see also the
    // discussion in step-18. Instead, we first build temporary objects that use
    // a different data structure that doesn't require allocating more memory
    // than necessary but isn't suitable for use as a basis of SparseMatrix or
    // BlockSparseMatrix objects; in a second step we then copy these objects
    // into objects of type BlockSparsityPattern. This is entirely analogous to
    // what we already did in step-11 and step-18. In particular, we make use of
    // the fact that we will never write into the $(1,1)$ block of the system
    // matrix and that this is the only block to be filled for the
    // preconditioner matrix.
    //
    // All this is done inside new scopes, which means that the memory of
    // <code>dsp</code> will be released once the information has been copied to
    // <code>sparsity_pattern</code>.
    {
      BlockDynamicSparsityPattern dsp(dofs_per_block, dofs_per_block);

      Table<2, DoFTools::Coupling> coupling(dim + 1, dim + 1);
      for (unsigned int c = 0; c < dim + 1; ++c)
        for (unsigned int d = 0; d < dim + 1; ++d)
          if (!((c == dim) && (d == dim)))
            coupling[c][d] = DoFTools::always;
          else
            coupling[c][d] = DoFTools::none;

      DoFTools::make_sparsity_pattern(
        dof_handler, coupling, dsp, constraints, false);

      sparsity_pattern.copy_from(dsp);
    }

    {
      BlockDynamicSparsityPattern preconditioner_dsp(dofs_per_block,
                                                     dofs_per_block);

      Table<2, DoFTools::Coupling> preconditioner_coupling(dim + 1, dim + 1);
      for (unsigned int c = 0; c < dim + 1; ++c)
        for (unsigned int d = 0; d < dim + 1; ++d)
          if (((c == dim) && (d == dim)))
            preconditioner_coupling[c][d] = DoFTools::always;
          else
            preconditioner_coupling[c][d] = DoFTools::none;

      DoFTools::make_sparsity_pattern(dof_handler,
                                      preconditioner_coupling,
                                      preconditioner_dsp,
                                      constraints,
                                      false);

      preconditioner_sparsity_pattern.copy_from(preconditioner_dsp);
    }

    // Finally, the system matrix, the preconsitioner matrix, the solution and
    // the right hand side vector are created from the block structure similar
    // to the approach in step-20:
    system_matrix.reinit(sparsity_pattern);
    preconditioner_matrix.reinit(preconditioner_sparsity_pattern);

    solution.reinit(dofs_per_block);
    system_rhs.reinit(dofs_per_block);
  }


  // @sect4{StokesProblem::assemble_system}

  // The assembly process follows the discussion in step-20 and in the
  // introduction. We use the well-known abbreviations for the data structures
  // that hold the local matrices, right hand side, and global numbering of the
  // degrees of freedom for the present cell.
  template <int dim>
  void StokesProblem<dim>::assemble_system()
  {
    system_matrix         = 0;
    system_rhs            = 0;
    preconditioner_matrix = 0;

    const QGauss<dim> quadrature_formula(degree + 2);

    FEValues<dim> fe_values(fe,
                            quadrature_formula,
                            update_values | update_quadrature_points |
                              update_JxW_values | update_gradients);

    const unsigned int dofs_per_cell = fe.n_dofs_per_cell();

    const unsigned int n_q_points = quadrature_formula.size();

    FullMatrix<double> local_matrix(dofs_per_cell, dofs_per_cell);
    FullMatrix<double> local_preconditioner_matrix(dofs_per_cell,
                                                   dofs_per_cell);
    Vector<double>     local_rhs(dofs_per_cell);

    std::vector<types::global_dof_index> local_dof_indices(dofs_per_cell);

    const RightHandSide<dim>    right_hand_side;
    std::vector<Tensor<1, dim>> rhs_values(n_q_points, Tensor<1, dim>());

    // Next, we need two objects that work as extractors for the FEValues
    // object. Their use is explained in detail in the report on @ref
    // vector_valued :
    const FEValuesExtractors::Vector velocities(0);
    const FEValuesExtractors::Scalar pressure(dim);

    // As an extension over step-20 and step-21, we include a few optimizations
    // that make assembly much faster for this particular problem. The
    // improvements are based on the observation that we do a few calculations
    // too many times when we do as in step-20: The symmetric gradient actually
    // has <code>dofs_per_cell</code> different values per quadrature point, but
    // we extract it <code>dofs_per_cell*dofs_per_cell</code> times from the
    // FEValues object - for both the loop over <code>i</code> and the inner
    // loop over <code>j</code>. In 3d, that means evaluating it $89^2=7921$
    // instead of $89$ times, a not insignificant difference.
    //
    // So what we're going to do here is to avoid such repeated calculations
    // by getting a vector of rank-2 tensors (and similarly for the divergence
    // and the basis function value on pressure) at the quadrature point prior
    // to starting the loop over the dofs on the cell. First, we create the
    // respective objects that will hold these values. Then, we start the loop
    // over all cells and the loop over the quadrature points, where we first
    // extract these values. There is one more optimization we implement here:
    // the local matrix (as well as the global one) is going to be symmetric,
    // since all the operations involved are symmetric with respect to $i$ and
    // $j$. This is implemented by simply running the inner loop not to
    // <code>dofs_per_cell</code>, but only up to <code>i</code>, the index of
    // the outer loop.
    std::vector<SymmetricTensor<2, dim>> symgrad_phi_u(dofs_per_cell);
    std::vector<double>                  div_phi_u(dofs_per_cell);
    std::vector<Tensor<1, dim>>          phi_u(dofs_per_cell);
    std::vector<double>                  phi_p(dofs_per_cell);

    for (const auto &cell : dof_handler.active_cell_iterators())
      {
        fe_values.reinit(cell);

        local_matrix                = 0;
        local_preconditioner_matrix = 0;
        local_rhs                   = 0;

        right_hand_side.value_list(fe_values.get_quadrature_points(),
                                   rhs_values);

        for (unsigned int q = 0; q < n_q_points; ++q)
          {
            for (unsigned int k = 0; k < dofs_per_cell; ++k)
              {
                symgrad_phi_u[k] =
                  fe_values[velocities].symmetric_gradient(k, q);
                div_phi_u[k] = fe_values[velocities].divergence(k, q);
                phi_u[k]     = fe_values[velocities].value(k, q);
                phi_p[k]     = fe_values[pressure].value(k, q);
              }

            // Now finally for the bilinear forms of both the system matrix and
            // the matrix we use for the preconditioner. Recall that the
            // formulas for these two are
            // @f{align*}{
            //   A_{ij} &= a(\varphi_i,\varphi_j)
            //   \\     &= \underbrace{2(\varepsilon(\varphi_{i,\textbf{u}}),
            //                           \varepsilon(\varphi_{j,\textbf{u}}))_{\Omega}}
            //                        _{(1)}
            //           \;
            //             \underbrace{- (\textrm{div}\; \varphi_{i,\textbf{u}},
            //                            \varphi_{j,p})_{\Omega}}
            //                        _{(2)}
            //           \;
            //             \underbrace{- (\varphi_{i,p},
            //                            \textrm{div}\;
            //                            \varphi_{j,\textbf{u}})_{\Omega}}
            //                        _{(3)}
            // @f}
            // and
            // @f{align*}{
            //   M_{ij} &= \underbrace{(\varphi_{i,p},
            //                          \varphi_{j,p})_{\Omega}}
            //                        _{(4)},
            // @f}
            // respectively, where $\varphi_{i,\textbf{u}}$ and $\varphi_{i,p}$
            // are the velocity and pressure components of the $i$th shape
            // function. The various terms above are then easily recognized in
            // the following implementation:
            for (unsigned int i = 0; i < dofs_per_cell; ++i)
              {
                for (unsigned int j = 0; j <= i; ++j)
                  {
                    local_matrix(i, j) +=
                      (2 * (symgrad_phi_u[i] * symgrad_phi_u[j]) // (1)
                       - div_phi_u[i] * phi_p[j]                 // (2)
                       - phi_p[i] * div_phi_u[j])                // (3)
                      * fe_values.JxW(q);                        // * dx

                    local_preconditioner_matrix(i, j) +=
                      (phi_p[i] * phi_p[j]) // (4)
                      * fe_values.JxW(q);   // * dx
                  }
                // Note that in the implementation of (1) above, `operator*`
                // is overloaded for symmetric tensors, yielding the scalar
                // product between the two tensors.
                //
                // For the right-hand side, we need to multiply the (vector of)
                // velocity shape functions with the vector of body force
                // right-hand side components, both evaluated at the current
                // quadrature point. We have implemented the body forces as a
                // `TensorFunction<1,dim>`, so its values at quadrature points
                // are already tensors for which the application of `operator*`
                // against the velocity components of the shape function results
                // in the dot product, as intended.
                local_rhs(i) += phi_u[i]            // phi_u_i(x_q)
                                * rhs_values[q]     // * f(x_q)
                                * fe_values.JxW(q); // * dx
              }
          }

        // Before we can write the local data into the global matrix (and
        // simultaneously use the AffineConstraints object to apply
        // Dirichlet boundary conditions and eliminate hanging node constraints,
        // as we discussed in the introduction), we have to be careful about one
        // thing, though. We have only built half of the local matrices
        // because of symmetry, but we're going to save the full matrices
        // in order to use the standard functions for solving. This is done
        // by flipping the indices in case we are pointing into the empty part
        // of the local matrices.
        for (unsigned int i = 0; i < dofs_per_cell; ++i)
          for (unsigned int j = i + 1; j < dofs_per_cell; ++j)
            {
              local_matrix(i, j) = local_matrix(j, i);
              local_preconditioner_matrix(i, j) =
                local_preconditioner_matrix(j, i);
            }

        cell->get_dof_indices(local_dof_indices);
        constraints.distribute_local_to_global(local_matrix,
                                               local_rhs,
                                               local_dof_indices,
                                               system_matrix,
                                               system_rhs);
        constraints.distribute_local_to_global(local_preconditioner_matrix,
                                               local_dof_indices,
                                               preconditioner_matrix);
      }

    // Before we're going to solve this linear system, we generate a
    // preconditioner for the velocity-velocity matrix, i.e.,
    // <code>block(0,0)</code> in the system matrix. As mentioned above, this
    // depends on the spatial dimension. Since the two classes described by
    // the <code>InnerPreconditioner::type</code> alias have the same
    // interface, we do not have to do anything different whether we want to
    // use a sparse direct solver or an ILU:
    std::cout << "   Computing preconditioner..." << std::endl << std::flush;

    A_preconditioner =
      std::make_shared<typename InnerPreconditioner<dim>::type>();
    A_preconditioner->initialize(
      system_matrix.block(0, 0),
      typename InnerPreconditioner<dim>::type::AdditionalData());
  }



  // @sect4{StokesProblem::solve}

  // After the discussion in the introduction and the definition of the
  // respective classes above, the implementation of the <code>solve</code>
  // function is rather straight-forward and done in a similar way as in
  // step-20. To start with, we need an object of the
  // <code>InverseMatrix</code> class that represents the inverse of the
  // matrix A. As described in the introduction, the inverse is generated with
  // the help of an inner preconditioner of type
  // <code>InnerPreconditioner::type</code>.
  template <int dim>
  void StokesProblem<dim>::solve()
  {
    const InverseMatrix<SparseMatrix<double>,
                        typename InnerPreconditioner<dim>::type>
                   A_inverse(system_matrix.block(0, 0), *A_preconditioner);
    Vector<double> tmp(solution.block(0).size());

    // This is as in step-20. We generate the right hand side $B A^{-1} F - G$
    // for the Schur complement and an object that represents the respective
    // linear operation $B A^{-1} B^T$, now with a template parameter
    // indicating the preconditioner - in accordance with the definition of
    // the class.
    {
      Vector<double> schur_rhs(solution.block(1).size());
      A_inverse.vmult(tmp, system_rhs.block(0));
      system_matrix.block(1, 0).vmult(schur_rhs, tmp);
      schur_rhs -= system_rhs.block(1);

      SchurComplement<typename InnerPreconditioner<dim>::type> schur_complement(
        system_matrix, A_inverse);

      // The usual control structures for the solver call are created...
      SolverControl            solver_control(solution.block(1).size(),
                                   1e-6 * schur_rhs.l2_norm());
      SolverCG<Vector<double>> cg(solver_control);

      // Now to the preconditioner to the Schur complement. As explained in
      // the introduction, the preconditioning is done by a @ref GlossMassMatrix "mass matrix" in the
      // pressure variable.
      //
      // Actually, the solver needs to have the preconditioner in the form
      // $P^{-1}$, so we need to create an inverse operation. Once again, we
      // use an object of the class <code>InverseMatrix</code>, which
      // implements the <code>vmult</code> operation that is needed by the
      // solver.  In this case, we have to invert the pressure mass matrix. As
      // it already turned out in earlier tutorial programs, the inversion of
      // a mass matrix is a rather cheap and straight-forward operation
      // (compared to, e.g., a Laplace matrix). The CG method with ILU
      // preconditioning converges in 5-10 steps, independently on the mesh
      // size.  This is precisely what we do here: We choose another ILU
      // preconditioner and take it along to the InverseMatrix object via the
      // corresponding template parameter.  A CG solver is then called within
      // the vmult operation of the inverse matrix.
      //
      // An alternative that is cheaper to build, but needs more iterations
      // afterwards, would be to choose a SSOR preconditioner with factor
      // 1.2. It needs about twice the number of iterations, but the costs for
      // its generation are almost negligible.
      SparseILU<double> preconditioner;
      preconditioner.initialize(preconditioner_matrix.block(1, 1),
                                SparseILU<double>::AdditionalData());

      InverseMatrix<SparseMatrix<double>, SparseILU<double>> m_inverse(
        preconditioner_matrix.block(1, 1), preconditioner);

      // With the Schur complement and an efficient preconditioner at hand, we
      // can solve the respective equation for the pressure (i.e. block 0 in
      // the solution vector) in the usual way:
      cg.solve(schur_complement, solution.block(1), schur_rhs, m_inverse);

      // After this first solution step, the hanging node constraints have to
      // be distributed to the solution in order to achieve a consistent
      // pressure field.
      constraints.distribute(solution);

      std::cout << "  " << solver_control.last_step()
                << " outer CG Schur complement iterations for pressure"
                << std::endl;
    }

    // As in step-20, we finally need to solve for the velocity equation where
    // we plug in the solution to the pressure equation. This involves only
    // objects we already know - so we simply multiply $p$ by $B^T$, subtract
    // the right hand side and multiply by the inverse of $A$. At the end, we
    // need to distribute the constraints from hanging nodes in order to
    // obtain a consistent flow field:
    {
      system_matrix.block(0, 1).vmult(tmp, solution.block(1));
      tmp *= -1;
      tmp += system_rhs.block(0);

      A_inverse.vmult(solution.block(0), tmp);

      constraints.distribute(solution);
    }
  }


  // @sect4{StokesProblem::output_results}

  // The next function generates graphical output. In this example, we are
  // going to use the VTK file format.  We attach names to the individual
  // variables in the problem: <code>velocity</code> to the <code>dim</code>
  // components of velocity and <code>pressure</code> to the pressure.
  //
  // Not all visualization programs have the ability to group individual
  // vector components into a vector to provide vector plots; in particular,
  // this holds for some VTK-based visualization programs. In this case, the
  // logical grouping of components into vectors should already be described
  // in the file containing the data. In other words, what we need to do is
  // provide our output writers with a way to know which of the components of
  // the finite element logically form a vector (with $d$ components in $d$
  // space dimensions) rather than letting them assume that we simply have a
  // bunch of scalar fields.  This is achieved using the members of the
  // <code>DataComponentInterpretation</code> namespace: as with the filename,
  // we create a vector in which the first <code>dim</code> components refer
  // to the velocities and are given the tag
  // DataComponentInterpretation::component_is_part_of_vector; we
  // finally push one tag
  // DataComponentInterpretation::component_is_scalar to describe
  // the grouping of the pressure variable.

  // The rest of the function is then the same as in step-20.
  template <int dim>
  void
  StokesProblem<dim>::output_results(const unsigned int refinement_cycle) const
  {
    std::vector<std::string> solution_names(dim, "velocity");
    solution_names.emplace_back("pressure");

    std::vector<DataComponentInterpretation::DataComponentInterpretation>
      data_component_interpretation(
        dim, DataComponentInterpretation::component_is_part_of_vector);
    data_component_interpretation.push_back(
      DataComponentInterpretation::component_is_scalar);

    DataOut<dim> data_out;
    data_out.attach_dof_handler(dof_handler);
    data_out.add_data_vector(solution,
                             solution_names,
                             DataOut<dim>::type_dof_data,
                             data_component_interpretation);
    data_out.build_patches();

    std::ofstream output(
      "solution-" + Utilities::int_to_string(refinement_cycle, 2) + ".vtk");
    data_out.write_vtk(output);
  }


  // @sect4{StokesProblem::refine_mesh}

  // This is the last interesting function of the <code>StokesProblem</code>
  // class.  As indicated by its name, it takes the solution to the problem
  // and refines the mesh where this is needed. The procedure is the same as
  // in the respective step in step-6, with the exception that we base the
  // refinement only on the change in pressure, i.e., we call the Kelly error
  // estimator with a mask object of type ComponentMask that selects the
  // single scalar component for the pressure that we are interested in (we
  // get such a mask from the finite element class by specifying the component
  // we want). Additionally, we do not coarsen the grid again:
  template <int dim>
  void StokesProblem<dim>::refine_mesh()
  {
    Vector<float> estimated_error_per_cell(triangulation.n_active_cells());

    const FEValuesExtractors::Scalar pressure(dim);
    KellyErrorEstimator<dim>::estimate(
      dof_handler,
      QGauss<dim - 1>(degree + 1),
      std::map<types::boundary_id, const Function<dim> *>(),
      solution,
      estimated_error_per_cell,
      fe.component_mask(pressure));

    GridRefinement::refine_and_coarsen_fixed_number(triangulation,
                                                    estimated_error_per_cell,
                                                    0.3,
                                                    0.0);
    triangulation.execute_coarsening_and_refinement();
  }


  // @sect4{StokesProblem::run}

  // The last step in the Stokes class is, as usual, the function that
  // generates the initial grid and calls the other functions in the
  // respective order.
  //
  // We start off with a rectangle of size $4 \times 1$ (in 2d) or $4 \times 1
  // \times 1$ (in 3d), placed in $R^2/R^3$ as $(-2,2)\times(-1,0)$ or
  // $(-2,2)\times(0,1)\times(-1,0)$, respectively. It is natural to start
  // with equal mesh size in each direction, so we subdivide the initial
  // rectangle four times in the first coordinate direction. To limit the
  // scope of the variables involved in the creation of the mesh to the range
  // where we actually need them, we put the entire block between a pair of
  // braces:
  template <int dim>
  void StokesProblem<dim>::run()
  {
    {
      std::vector<unsigned int> subdivisions(dim, 1);
      subdivisions[0] = 4;

      const Point<dim> bottom_left = (dim == 2 ?                //
                                        Point<dim>(-2, -1) :    // 2d case
                                        Point<dim>(-2, 0, -1)); // 3d case

      const Point<dim> top_right = (dim == 2 ?              //
                                      Point<dim>(2, 0) :    // 2d case
                                      Point<dim>(2, 1, 0)); // 3d case

      GridGenerator::subdivided_hyper_rectangle(triangulation,
                                                subdivisions,
                                                bottom_left,
                                                top_right);
    }

    // A boundary indicator of 1 is set to all boundaries that are subject to
    // Dirichlet boundary conditions, i.e.  to faces that are located at 0 in
    // the last coordinate direction. See the example description above for
    // details.
    for (const auto &cell : triangulation.active_cell_iterators())
      for (const auto &face : cell->face_iterators())
        if (face->center()[dim - 1] == 0)
          face->set_all_boundary_ids(1);


    // We then apply an initial refinement before solving for the first
    // time. In 3d, there are going to be more degrees of freedom, so we
    // refine less there:
    triangulation.refine_global(4 - dim);

    // As first seen in step-6, we cycle over the different refinement levels
    // and refine (except for the first cycle), setup the degrees of freedom
    // and matrices, assemble, solve and create output:
    for (unsigned int refinement_cycle = 0; refinement_cycle < 6;
         ++refinement_cycle)
      {
        std::cout << "Refinement cycle " << refinement_cycle << std::endl;

        if (refinement_cycle > 0)
          refine_mesh();

        setup_dofs();

        std::cout << "   Assembling..." << std::endl << std::flush;
        assemble_system();

        std::cout << "   Solving..." << std::flush;
        solve();

        output_results(refinement_cycle);

        std::cout << std::endl;
      }
  }
} // namespace Step22


// @sect3{The <code>main</code> function}

// The main function is the same as in step-20. We pass the element degree as
// a parameter and choose the space dimension at the well-known template slot.
int main()
{
  try
    {
      using namespace Step22;

      StokesProblem<2> flow_problem(1);
      flow_problem.run();
    }
  catch (std::exception &exc)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Exception on processing: " << std::endl
                << exc.what() << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;

      return 1;
    }
  catch (...)
    {
      std::cerr << std::endl
                << std::endl
                << "----------------------------------------------------"
                << std::endl;
      std::cerr << "Unknown exception!" << std::endl
                << "Aborting!" << std::endl
                << "----------------------------------------------------"
                << std::endl;
      return 1;
    }

  return 0;
}
