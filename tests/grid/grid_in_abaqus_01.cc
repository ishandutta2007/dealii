// ---------------------------------------------------------------------
//
// Copyright (C) 2004 - 2021 by the deal.II authors
//
// This file is part of the deal.II library.
//
// The deal.II library is free software; you can use it, redistribute
// it, and/or modify it under the terms of the GNU Lesser General
// Public License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// The full text of the license can be found in the file LICENSE.md at
// the top level directory of deal.II.
//
// ---------------------------------------------------------------------



// check whether we can read in with the abaqus format

#include <deal.II/grid/grid_in.h>
#include <deal.II/grid/tria.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>

#include "../tests.h"



template <int dim>
void
abaqus_grid(const char *name)
{
  Triangulation<dim> tria;
  GridIn<dim>        grid_in;
  grid_in.attach_triangulation(tria);
  std::ifstream input_file(name);
  grid_in.read_abaqus(input_file);

  deallog << "  " << tria.n_active_cells() << " active cells" << std::endl;

  int hash  = 0;
  int index = 0;
  for (typename Triangulation<dim>::active_cell_iterator c =
         tria.begin_active();
       c != tria.end();
       ++c, ++index)
    for (const unsigned int i : c->vertex_indices())
      hash += (index * i * c->vertex_index(i)) % (tria.n_active_cells() + 1);
  deallog << "  hash=" << hash << std::endl;
}


int
main()
{
  initlog();

  try
    {
      deallog << "2d_test.inp" << std::endl;
      abaqus_grid<2>(SOURCE_DIR "/grids/abaqus/2d/2d_test.inp");
      deallog << "2d_quad.inp" << std::endl;
      abaqus_grid<2>(SOURCE_DIR "/grids/abaqus/2d/2d_quad.inp");
      //      deallog << "2d_test_pave.inp" << std::endl;
      //      abaqus_grid<2> (SOURCE_DIR "/grids/abaqus/2d/2d_test_pave.inp");
      //      // Failing test
      deallog << "2d_test_abaqus.inp" << std::endl;
      abaqus_grid<2>(SOURCE_DIR "/grids/abaqus/2d/2d_test_abaqus.inp");
      deallog << "2d_short_handwritten.inp" << std::endl;
      abaqus_grid<2>(SOURCE_DIR "/grids/abaqus/2d/2d_short_handwritten.inp");

      deallog << "3d_test_cube_1.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR "/grids/abaqus/3d/3d_test_cube_1.inp");
      deallog << "3d_test_cube_two_materials.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR
                     "/grids/abaqus/3d/3d_test_cube_two_materials.inp");
      deallog << "3d_CC_cubit_old.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR "/grids/abaqus/3d/3d_CC_cubit_old.inp");
      deallog << "3d_CC_cubit_new.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR "/grids/abaqus/3d/3d_CC_cubit_new.inp");
      deallog << "3d_test_cube_pave_1.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR "/grids/abaqus/3d/3d_test_cube_pave_1.inp");
      deallog << "3d_other_simple.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR "/grids/abaqus/3d/3d_other_simple.inp");
      deallog << "3d_test_abaqus.inp" << std::endl;
      abaqus_grid<3>(SOURCE_DIR "/grids/abaqus/3d/3d_test_abaqus.inp");
    }
  catch (const std::exception &exc)
    {
      deallog << std::endl
              << std::endl
              << "----------------------------------------------------"
              << std::endl;
      deallog << "Exception on processing: " << std::endl
              << exc.what() << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------"
              << std::endl;
      return 1;
    }
  catch (...)
    {
      deallog << std::endl
              << std::endl
              << "----------------------------------------------------"
              << std::endl;
      deallog << "Unknown exception!" << std::endl
              << "Aborting!" << std::endl
              << "----------------------------------------------------"
              << std::endl;
      return 1;
    };

  return 0;
}
