// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2010 - 2024 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------



// check AffineConstraints<double>.distribute() for a distributed mesh
// with Trilinos; manual check of the graphical output...
// Mesh: shell with random refinement

#include <deal.II/base/function.h>
#include <deal.II/base/tensor.h>

#include <deal.II/distributed/tria.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>

#include <deal.II/grid/filtered_iterator.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_out.h>
#include <deal.II/grid/manifold_lib.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include <deal.II/lac/trilinos_vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/solution_transfer.h>
#include <deal.II/numerics/vector_tools.h>

#include "../tests.h"

const double R0 = 0.5; // 6371000.-2890000.;
const double R1 = 1.0; // 6371000.-  35000.;
const double T0 = 1.0;
const double T1 = 2.0;



template <int dim>
class TemperatureInitialValues : public Function<dim>
{
public:
  TemperatureInitialValues()
    : Function<dim>(1)
  {}

  virtual double
  value(const Point<dim> &p, const unsigned int component = 0) const;

  virtual void
  vector_value(const Point<dim> &p, Vector<double> &value) const;
};



template <int dim>
double
TemperatureInitialValues<dim>::value(const Point<dim> &p,
                                     const unsigned int) const
{
  return p[0] * T1 + p[1] * (T0 - T1); // simple
}


template <int dim>
void
TemperatureInitialValues<dim>::vector_value(const Point<dim> &p,
                                            Vector<double>   &values) const
{
  for (unsigned int c = 0; c < this->n_components; ++c)
    values(c) = TemperatureInitialValues<dim>::value(p, c);
}


template <int dim>
void
test()
{
  parallel::distributed::Triangulation<dim> tr(MPI_COMM_WORLD);


  //  GridGenerator::hyper_cube(tr);


  GridGenerator::hyper_shell(tr, Point<dim>(), R0, R1, 12, true);
  tr.reset_all_manifolds();
  static SphericalManifold<dim> boundary;
  //  tr.set_manifold (0, boundary);
  // tr.set_manifold (1, boundary);

  tr.refine_global(3);
  if (1)
    for (unsigned int step = 0; step < 5; ++step)
      {
        typename Triangulation<dim>::active_cell_iterator cell =
                                                            tr.begin_active(),
                                                          endc = tr.end();

        for (; cell != endc; ++cell)
          if (Testing::rand() % 42 == 1)
            cell->set_refine_flag();

        tr.execute_coarsening_and_refinement();
      }

  DoFHandler<dim> dofh(tr);

  static FE_Q<dim> fe(2);

  dofh.distribute_dofs(fe);

  IndexSet owned_set    = dofh.locally_owned_dofs();
  IndexSet dof_set      = DoFTools::extract_locally_active_dofs(dofh);
  IndexSet relevant_set = DoFTools::extract_locally_relevant_dofs(dofh);

  TrilinosWrappers::MPI::Vector x;
  x.reinit(owned_set, MPI_COMM_WORLD);

  VectorTools::interpolate(dofh, TemperatureInitialValues<dim>(), x);
  TrilinosWrappers::MPI::Vector x_rel;
  x_rel.reinit(relevant_set, MPI_COMM_WORLD);
  x_rel = x;

  for (unsigned int steps = 0; steps < 3; ++steps)
    {
      {
        typename Triangulation<dim>::active_cell_iterator cell =
                                                            tr.begin_active(),
                                                          endc = tr.end();

        for (; cell != endc; ++cell)
          if (!cell->is_artificial() && !cell->is_ghost())
            {
              if (Testing::rand() % 12 == 1)
                cell->set_refine_flag();
              else if (Testing::rand() % 7 == 1)
                cell->set_coarsen_flag();
            }
      }
      for (typename Triangulation<dim>::cell_iterator cell = tr.begin();
           cell != tr.end();
           ++cell)
        {
          if (!cell->has_children())
            continue;

          bool coarsen_me = false;
          for (unsigned int i = 0; i < cell->n_children(); ++i)
            if (cell->child(i)->coarsen_flag_set())
              {
                coarsen_me = true;
                break;
              }
          if (coarsen_me)
            for (unsigned int i = 0; i < cell->n_children(); ++i)
              {
                if (cell->child(i)->is_active() &&
                    cell->child(i)->is_locally_owned())
                  {
                    cell->child(i)->clear_refine_flag();
                    cell->child(i)->set_coarsen_flag();
                  }
              }
        }



      SolutionTransfer<dim, TrilinosWrappers::MPI::Vector> trans(dofh);
      tr.prepare_coarsening_and_refinement();


      trans.prepare_for_coarsening_and_refinement(x_rel);

      tr.execute_coarsening_and_refinement();

      static FE_Q<dim> fe(2);

      dofh.distribute_dofs(fe);

      owned_set    = dofh.locally_owned_dofs();
      dof_set      = DoFTools::extract_locally_active_dofs(dofh);
      relevant_set = DoFTools::extract_locally_relevant_dofs(dofh);

      x.reinit(owned_set, MPI_COMM_WORLD);

      trans.interpolate(x);

      x_rel.reinit(relevant_set, MPI_COMM_WORLD);

      AffineConstraints<double> cm(owned_set, relevant_set);
      DoFTools::make_hanging_node_constraints(dofh, cm);
      /*  std::vector<bool> velocity_mask (dim+1, true);

          velocity_mask[dim] = false;

          VectorTools::interpolate_boundary_values (dofh,
          0,
          Functions::ZeroFunction<dim>(1),
          cm,
          velocity_mask);          */

      cm.close();

      cm.distribute(x);
      x_rel = x;
    }


  TrilinosWrappers::MPI::Vector x_ref;
  x_ref.reinit(owned_set, MPI_COMM_WORLD);

  VectorTools::interpolate(dofh, TemperatureInitialValues<dim>(), x_ref);

  x_ref -= x;
  double err = x_ref.linfty_norm();
  if (err > 1.0e-12)
    if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
      deallog << "err:" << err << std::endl;

  //  x_rel=x_ref; //uncomment to output error

  std::vector<std::string> solution_names(1, "T");

  DataOut<dim> data_out;
  data_out.set_cell_selection(
    [](const Triangulation<dim> &t) {
      auto cell = t.begin_active();
      while ((cell != t.end()) &&
             (cell->subdomain_id() != t.locally_owned_subdomain()))
        ++cell;

      return cell;
    },

    [](const Triangulation<dim>                         &t,
       const typename Triangulation<dim>::cell_iterator &old_cell) ->
    typename Triangulation<dim>::cell_iterator {
      if (old_cell != t.end())
        {
          const IteratorFilters::SubdomainEqualTo predicate(
            t.locally_owned_subdomain());

          return ++(
            FilteredIterator<typename Triangulation<dim>::active_cell_iterator>(
              predicate, old_cell));
        }
      else
        return old_cell;
    });
  data_out.attach_dof_handler(dofh);

  data_out.add_data_vector(x_rel, solution_names);

  data_out.build_patches(1);
  const std::string filename =
    ("solution." + Utilities::int_to_string(tr.locally_owned_subdomain(), 4) +
     ".d2");
  std::ofstream output(filename);
  data_out.write_deal_II_intermediate(output);

  tr.reset_manifold(0);
  tr.reset_manifold(1);
  if (Utilities::MPI::this_mpi_process(MPI_COMM_WORLD) == 0)
    deallog << "OK" << std::endl;
}


int
main(int argc, char *argv[])
{
  Utilities::MPI::MPI_InitFinalize mpi_initialization(argc, argv, 1);

  unsigned int myid = Utilities::MPI::this_mpi_process(MPI_COMM_WORLD);


  deallog.push(Utilities::int_to_string(myid));

  if (myid == 0)
    {
      initlog();

      deallog.push("2d");
      test<2>();
      deallog.pop();
    }
  else
    test<2>();
}
