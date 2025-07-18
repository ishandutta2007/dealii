// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2008 - 2025 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------

#include <deal.II/base/quadrature_lib.h>
#include <deal.II/base/utilities.h>

#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/dofs/dof_handler.h>

#include <deal.II/fe/fe.h>
#include <deal.II/fe/fe_tools.h>
#include <deal.II/fe/mapping_q_eulerian.h>

#include <deal.II/grid/tria_iterator.h>

#include <deal.II/lac/block_vector.h>
#include <deal.II/lac/la_parallel_block_vector.h>
#include <deal.II/lac/la_parallel_vector.h>
#include <deal.II/lac/petsc_block_vector.h>
#include <deal.II/lac/petsc_vector.h>
#include <deal.II/lac/trilinos_parallel_block_vector.h>
#include <deal.II/lac/trilinos_vector.h>
#include <deal.II/lac/vector.h>

#include <boost/container/small_vector.hpp>

#include <memory>


DEAL_II_NAMESPACE_OPEN



template <int dim, typename VectorType, int spacedim>
MappingQEulerian<dim, VectorType, spacedim>::MappingQEulerian(
  const unsigned int               degree,
  const DoFHandler<dim, spacedim> &euler_dof_handler,
  const VectorType                &euler_vector,
  const unsigned int               level)
  : MappingQ<dim, spacedim>(degree)
  , euler_vector(&euler_vector)
  , euler_dof_handler(&euler_dof_handler)
  , level(level)
  , support_quadrature(degree)
  , mapping_q(degree)
  , fe_values(mapping_q,
              euler_dof_handler.get_fe(),
              support_quadrature,
              update_values | update_quadrature_points)
{}



template <int dim, typename VectorType, int spacedim>
std::unique_ptr<Mapping<dim, spacedim>>
MappingQEulerian<dim, VectorType, spacedim>::clone() const
{
  return std::make_unique<MappingQEulerian<dim, VectorType, spacedim>>(
    this->get_degree(), *euler_dof_handler, *euler_vector, this->level);
}



template <int dim, typename VectorType, int spacedim>
MappingQEulerian<dim, VectorType, spacedim>::SupportQuadrature::
  SupportQuadrature(const unsigned int map_degree)
  : Quadrature<dim>(Utilities::fixed_power<dim>(map_degree + 1))
{
  // first we determine the support points on the unit cell in lexicographic
  // order, which are (in accordance with MappingQ) the support points of
  // QGaussLobatto.
  const QGaussLobatto<dim> q_iterated(map_degree + 1);
  const unsigned int       n_q_points = q_iterated.size();

  // we then need to define a renumbering vector that allows us to go from a
  // lexicographic numbering scheme to a hierarchic one.
  const std::vector<unsigned int> renumber =
    FETools::lexicographic_to_hierarchic_numbering<dim>(map_degree);

  // finally we assign the quadrature points in the required order.
  for (unsigned int q = 0; q < n_q_points; ++q)
    this->quadrature_points[renumber[q]] = q_iterated.point(q);
}



template <int dim, typename VectorType, int spacedim>
boost::container::small_vector<Point<spacedim>,
#ifndef _MSC_VER
                               ReferenceCells::max_n_vertices<dim>()
#else
                               GeometryInfo<dim>::vertices_per_cell
#endif
                               >
MappingQEulerian<dim, VectorType, spacedim>::get_vertices(
  const typename Triangulation<dim, spacedim>::cell_iterator &cell) const
{
  // get the vertices as the first 2^dim mapping support points
  const std::vector<Point<spacedim>> a = compute_mapping_support_points(cell);

  boost::container::small_vector<Point<spacedim>,
#ifndef _MSC_VER
                                 ReferenceCells::max_n_vertices<dim>()
#else
                                 GeometryInfo<dim>::vertices_per_cell
#endif
                                 >
    vertex_locations(a.begin(), a.begin() + cell->n_vertices());

  return vertex_locations;
}



template <int dim, typename VectorType, int spacedim>
std::vector<Point<spacedim>>
MappingQEulerian<dim, VectorType, spacedim>::compute_mapping_support_points(
  const typename Triangulation<dim, spacedim>::cell_iterator &cell) const
{
  const bool mg_vector = level != numbers::invalid_unsigned_int;

  [[maybe_unused]] const types::global_dof_index n_dofs =
    mg_vector ? euler_dof_handler->n_dofs(level) : euler_dof_handler->n_dofs();
  [[maybe_unused]] const types::global_dof_index vector_size =
    euler_vector->size();

  AssertDimension(vector_size, n_dofs);

  // we then transform our tria iterator into a dof iterator so we can access
  // data not associated with triangulations
  typename DoFHandler<dim, spacedim>::cell_iterator dof_cell(*cell,
                                                             euler_dof_handler);

  Assert(mg_vector || dof_cell->is_active() == true, ExcInactiveCell());

  // our quadrature rule is chosen so that each quadrature point corresponds
  // to a support point in the undeformed configuration. We can then query
  // the given displacement field at these points to determine the shift
  // vector that maps the support points to the deformed configuration.

  // We assume that the given field contains dim displacement components, but
  // that there may be other solution components as well (e.g. pressures).
  // this class therefore assumes that the first dim components represent the
  // actual shift vector we need, and simply ignores any components after
  // that. This implies that the user should order components appropriately,
  // or create a separate dof handler for the displacements.
  const unsigned int n_support_pts = support_quadrature.size();
  const unsigned int n_components = euler_dof_handler->get_fe(0).n_components();

  Assert(n_components >= spacedim,
         ExcDimensionMismatch(n_components, spacedim));

  std::vector<Vector<typename VectorType::value_type>> shift_vector(
    n_support_pts, Vector<typename VectorType::value_type>(n_components));

  std::vector<types::global_dof_index> dof_indices(
    euler_dof_handler->get_fe(0).n_dofs_per_cell());
  // fill shift vector for each support point using an fe_values object. make
  // sure that the fe_values variable isn't used simultaneously from different
  // threads
  std::lock_guard<std::mutex> lock(fe_values_mutex);
  fe_values.reinit(dof_cell);
  if (mg_vector)
    {
      dof_cell->get_mg_dof_indices(dof_indices);
      fe_values.get_function_values(*euler_vector, dof_indices, shift_vector);
    }
  else
    fe_values.get_function_values(*euler_vector, shift_vector);

  // and finally compute the positions of the support points in the deformed
  // configuration.
  std::vector<Point<spacedim>> a(n_support_pts);
  for (unsigned int q = 0; q < n_support_pts; ++q)
    {
      a[q] = fe_values.quadrature_point(q);
      for (unsigned int d = 0; d < spacedim; ++d)
        a[q][d] += shift_vector[q][d];
    }

  return a;
}



template <int dim, typename VectorType, int spacedim>
CellSimilarity::Similarity
MappingQEulerian<dim, VectorType, spacedim>::fill_fe_values(
  const typename Triangulation<dim, spacedim>::cell_iterator &cell,
  const CellSimilarity::Similarity,
  const Quadrature<dim>                                   &quadrature,
  const typename Mapping<dim, spacedim>::InternalDataBase &internal_data,
  internal::FEValuesImplementation::MappingRelatedData<dim, spacedim>
    &output_data) const
{
  // call the function of the base class, but ignoring
  // any potentially detected cell similarity between
  // the current and the previous cell
  MappingQ<dim, spacedim>::fill_fe_values(cell,
                                          CellSimilarity::invalid_next_cell,
                                          quadrature,
                                          internal_data,
                                          output_data);
  // also return the updated flag since any detected
  // similarity wasn't based on the mapped field, but
  // the original vertices which are meaningless
  return CellSimilarity::invalid_next_cell;
}


// explicit instantiations
#include "fe/mapping_q_eulerian.inst"


DEAL_II_NAMESPACE_CLOSE
