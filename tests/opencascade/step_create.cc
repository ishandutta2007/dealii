// ---------------------------------------------------------------------
//
// Copyright (C) 2014 - 2021 by the deal.II authors
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


// Create a bspline, and write it as an IGES file.

#include <deal.II/opencascade/utilities.h>

#include <Standard_Stream.hxx>
#include <TopTools.hxx>
#include <TopoDS_Shape.hxx>

#include "../tests.h"

using namespace OpenCASCADE;

int
main()
{
  // Create a bspline passing through the points
  std::vector<Point<3>> pts;
  pts.push_back(Point<3>(0, 0, 0));
  pts.push_back(Point<3>(0, 1, 0));
  pts.push_back(Point<3>(1, 1, 0));
  pts.push_back(Point<3>(1, 0, 0));

  TopoDS_Edge edge = interpolation_curve(pts);
  write_STEP(edge, "tmp.step");
  std::ifstream in("tmp.step");
  std::ofstream out("output");
  std::string   line;
  unsigned int  counter = 0;

  while (std::getline(in, line))
    {
      counter++;
      if ((counter == 4) || (counter == 5) || (counter == 6) ||
          (counter == 18) || (counter == 19))
        {
        }
      else
        out << line << std::endl;
    }
  in.close();
  out.close();
  return 0;
}
