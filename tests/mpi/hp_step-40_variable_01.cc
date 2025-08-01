// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2012 - 2024 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------


// A lightly adapted version of the step-40 tutorial program. compared
// to the plain mpi/step-40 test, this test uses hp::DoFHandler, and
// compared to the hp_step-40 test, it uses different finite element
// objects (though all of the same kind)

#include <deal.II/base/conditional_ostream.h>
#include <deal.II/base/function.h>
#include <deal.II/base/index_set.h>
#include <deal.II/base/mpi.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/utilities.h>

#include <deal.II/distributed/grid_refinement.h>
#include <deal.II/distributed/tria.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/hp/fe_collection.h>
#include <deal.II/hp/fe_values.h>

#include <deal.II/lac/affine_constraints.h>
#include <deal.II/lac/dynamic_sparsity_pattern.h>
#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/petsc_precondition.h>
#include <deal.II/lac/petsc_solver.h>
#include <deal.II/lac/petsc_sparse_matrix.h>
#include <deal.II/lac/petsc_vector.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/sparsity_tools.h>
#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/error_estimator.h>
#include <deal.II/numerics/vector_tools.h>

#include <iostream>

#include "../tests.h"

namespace Step40
{
  template <int dim>
  class LaplaceProblem
  {
  public:
    LaplaceProblem();
    ~LaplaceProblem();

    void
    run();

  private:
    void
    setup_system();
    void
    assemble_system();
    void
    solve();
    void
    refine_grid();

    MPI_Comm mpi_communicator;

    parallel::distributed::Triangulation<dim> triangulation;

    DoFHandler<dim>       dof_handler;
    hp::FECollection<dim> fe;

    IndexSet locally_owned_dofs;
    IndexSet locally_relevant_dofs;

    AffineConstraints<PetscScalar> constraints;

    PETScWrappers::MPI::SparseMatrix system_matrix;
    PETScWrappers::MPI::Vector       locally_relevant_solution;
    PETScWrappers::MPI::Vector       system_rhs;

    ConditionalOStream pcout;
  };



  template <int dim>
  LaplaceProblem<dim>::LaplaceProblem()
    : mpi_communicator(MPI_COMM_WORLD)
    , triangulation(mpi_communicator,
                    typename Triangulation<dim>::MeshSmoothing(
                      Triangulation<dim>::smoothing_on_refinement |
                      Triangulation<dim>::smoothing_on_coarsening))
    , dof_handler(triangulation)
    , pcout(Utilities::MPI::this_mpi_process(mpi_communicator) == 0 ?
              deallog.get_file_stream() :
              std::cout,
            (Utilities::MPI::this_mpi_process(mpi_communicator) == 0))
  {
    fe.push_back(FE_Q<dim>(2));
    fe.push_back(FE_Q<dim>(2));
    fe.push_back(FE_Q<dim>(2));
  }



  template <int dim>
  LaplaceProblem<dim>::~LaplaceProblem()
  {
    dof_handler.clear();
  }



  template <int dim>
  void
  LaplaceProblem<dim>::setup_system()
  {
    // set active_fe_index mostly randomly
    for (const auto &cell : dof_handler.active_cell_iterators() |
                              IteratorFilters::LocallyOwnedCell())
      cell->set_active_fe_index(cell->active_cell_index() % fe.size());

    dof_handler.distribute_dofs(fe);

    locally_owned_dofs = dof_handler.locally_owned_dofs();
    locally_relevant_dofs =
      DoFTools::extract_locally_relevant_dofs(dof_handler);

    locally_relevant_solution.reinit(locally_owned_dofs,
                                     locally_relevant_dofs,
                                     mpi_communicator);
    system_rhs.reinit(locally_owned_dofs, mpi_communicator);
    system_rhs = PetscScalar();

    constraints.clear();
    constraints.reinit(locally_owned_dofs, locally_relevant_dofs);
    DoFTools::make_hanging_node_constraints(dof_handler, constraints);
    VectorTools::interpolate_boundary_values(
      dof_handler, 0, Functions::ZeroFunction<dim, PetscScalar>(), constraints);
    constraints.close();

    DynamicSparsityPattern csp(dof_handler.n_dofs(),
                               dof_handler.n_dofs(),
                               locally_relevant_dofs);
    DoFTools::make_sparsity_pattern(dof_handler, csp, constraints, false);
    SparsityTools::distribute_sparsity_pattern(csp,
                                               locally_owned_dofs,
                                               mpi_communicator,
                                               locally_relevant_dofs);
    system_matrix.reinit(
      mpi_communicator,
      csp,
      Utilities::MPI::all_gather(MPI_COMM_WORLD,
                                 dof_handler.n_locally_owned_dofs()),
      Utilities::MPI::all_gather(MPI_COMM_WORLD,
                                 dof_handler.n_locally_owned_dofs()),
      Utilities::MPI::this_mpi_process(mpi_communicator));
  }



  template <int dim>
  void
  LaplaceProblem<dim>::assemble_system()
  {
    const QGauss<dim>    quadrature_formula(3);
    hp::QCollection<dim> q_collection;
    q_collection.push_back(quadrature_formula);

    hp::FEValues<dim> x_fe_values(fe,
                                  q_collection,
                                  update_values | update_gradients |
                                    update_quadrature_points |
                                    update_JxW_values);

    FullMatrix<PetscScalar> cell_matrix;
    Vector<PetscScalar>     cell_rhs;

    std::vector<types::global_dof_index> local_dof_indices;

    typename DoFHandler<dim>::active_cell_iterator cell =
                                                     dof_handler.begin_active(),
                                                   endc = dof_handler.end();
    for (; cell != endc; ++cell)
      if (cell->is_locally_owned())
        {
          x_fe_values.reinit(cell);
          const FEValues<dim> &fe_values = x_fe_values.get_present_fe_values();

          const unsigned int dofs_per_cell = cell->get_fe().dofs_per_cell;
          const unsigned int n_q_points    = fe_values.get_quadrature().size();

          cell_matrix.reinit(dofs_per_cell, dofs_per_cell);
          cell_rhs.reinit(dofs_per_cell);

          local_dof_indices.resize(dofs_per_cell);

          cell_matrix = PetscScalar();
          cell_rhs    = PetscScalar();

          for (unsigned int q_point = 0; q_point < n_q_points; ++q_point)
            {
              const double rhs_value =
                (fe_values.quadrature_point(q_point)[1] >
                     0.5 +
                       0.25 * std::sin(4.0 * numbers::PI *
                                       fe_values.quadrature_point(q_point)[0]) ?
                   1 :
                   -1);

              for (unsigned int i = 0; i < dofs_per_cell; ++i)
                {
                  for (unsigned int j = 0; j < dofs_per_cell; ++j)
                    cell_matrix(i, j) += (fe_values.shape_grad(i, q_point) *
                                          fe_values.shape_grad(j, q_point) *
                                          fe_values.JxW(q_point));

                  cell_rhs(i) +=
                    (rhs_value * fe_values.shape_value(i, q_point) *
                     fe_values.JxW(q_point));
                }
            }

          cell->get_dof_indices(local_dof_indices);
          constraints.distribute_local_to_global(cell_matrix,
                                                 cell_rhs,
                                                 local_dof_indices,
                                                 system_matrix,
                                                 system_rhs);
        }

    system_matrix.compress(VectorOperation::add);
    system_rhs.compress(VectorOperation::add);
  }



  template <int dim>
  void
  LaplaceProblem<dim>::solve()
  {
    PETScWrappers::MPI::Vector completely_distributed_solution(
      mpi_communicator,
      dof_handler.n_dofs(),
      dof_handler.n_locally_owned_dofs());

    SolverControl solver_control(dof_handler.n_dofs(), 1e-12);

    PETScWrappers::SolverCG solver(solver_control);

#ifndef PETSC_USE_COMPLEX
    PETScWrappers::PreconditionBoomerAMG preconditioner(
      system_matrix,
      PETScWrappers::PreconditionBoomerAMG::AdditionalData(true));

    check_solver_within_range(solver.solve(system_matrix,
                                           completely_distributed_solution,
                                           system_rhs,
                                           preconditioner),
                              solver_control.last_step(),
                              8,
                              12);
#else
    check_solver_within_range(solver.solve(system_matrix,
                                           completely_distributed_solution,
                                           system_rhs,
                                           PETScWrappers::PreconditionJacobi(
                                             system_matrix)),
                              solver_control.last_step(),
                              120,
                              260);
#endif

    constraints.distribute(completely_distributed_solution);

    locally_relevant_solution = completely_distributed_solution;
  }



  template <int dim>
  void
  LaplaceProblem<dim>::refine_grid()
  {
    triangulation.refine_global(1);
  }



  template <int dim>
  void
  LaplaceProblem<dim>::run()
  {
    const unsigned int n_cycles = 2;
    for (unsigned int cycle = 0; cycle < n_cycles; ++cycle)
      {
        pcout << "Cycle " << cycle << ':' << std::endl;

        if (cycle == 0)
          {
            GridGenerator::hyper_cube(triangulation);
            triangulation.refine_global(5);
          }
        else
          refine_grid();

        setup_system();

        pcout << "   Number of active cells:       "
              << triangulation.n_global_active_cells() << std::endl
              << "      ";
        const auto n_locally_owned_active_cells_per_processor =
          Utilities::MPI::all_gather(
            triangulation.get_mpi_communicator(),
            triangulation.n_locally_owned_active_cells());
        for (unsigned int i = 0;
             i < Utilities::MPI::n_mpi_processes(mpi_communicator);
             ++i)
          pcout << n_locally_owned_active_cells_per_processor[i] << '+';
        pcout << std::endl;

        pcout << "   Number of degrees of freedom: " << dof_handler.n_dofs()
              << std::endl
              << "      ";
        for (unsigned int i = 0;
             i < Utilities::MPI::n_mpi_processes(mpi_communicator);
             ++i)
          pcout << Utilities::MPI::all_gather(
                     MPI_COMM_WORLD, dof_handler.n_locally_owned_dofs())[i]
                << '+';
        pcout << std::endl;

        assemble_system();
        solve();

        pcout << std::endl;
      }
  }
} // namespace Step40


int
test_mpi()
{
  try
    {
      using namespace Step40;


      {
        LaplaceProblem<2> laplace_problem_2d;
        laplace_problem_2d.run();
      }
    }
  catch (const std::exception &exc)
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



int
main(int argc, char *argv[])
{
  Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, 1);

  if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
    {
      initlog();

      deallog.push("mpi");
      test_mpi();
      deallog.pop();
    }
  else
    test_mpi();
}
