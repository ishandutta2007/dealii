// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2017 - 2025 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------

#include <deal.II/base/bounding_box.h>
#include <deal.II/base/geometry_info.h>

#include <limits>
#include <numeric>

DEAL_II_NAMESPACE_OPEN

template <int spacedim, typename Number>
bool
BoundingBox<spacedim, Number>::point_inside(const Point<spacedim, Number> &p,
                                            const double tolerance) const
{
  for (unsigned int i = 0; i < spacedim; ++i)
    {
      // Bottom left-top right convention: the point is outside if it's smaller
      // than the first or bigger than the second boundary point The bounding
      // box is defined as a closed set
      if ((p[i] <
           this->boundary_points.first[i] - tolerance * side_length(i)) ||
          (p[i] > this->boundary_points.second[i] + tolerance * side_length(i)))
        return false;
    }
  return true;
}



template <int spacedim, typename Number>
void
BoundingBox<spacedim, Number>::merge_with(
  const BoundingBox<spacedim, Number> &other_bbox)
{
  for (unsigned int i = 0; i < spacedim; ++i)
    {
      this->boundary_points.first[i] =
        std::min(this->boundary_points.first[i],
                 other_bbox.boundary_points.first[i]);
      this->boundary_points.second[i] =
        std::max(this->boundary_points.second[i],
                 other_bbox.boundary_points.second[i]);
    }
}

template <int spacedim, typename Number>
bool
BoundingBox<spacedim, Number>::has_overlap_with(
  const BoundingBox<spacedim, Number> &other_bbox,
  const double                         tolerance) const
{
  for (unsigned int i = 0; i < spacedim; ++i)
    {
      // testing if the boxes are close enough to intersect
      if ((other_bbox.boundary_points.second[i] <
           this->boundary_points.first[i] - tolerance * side_length(i)) ||
          (other_bbox.boundary_points.first[i] >
           this->boundary_points.second[i] + tolerance * side_length(i)))
        return false;
    }
  return true;
}

template <int spacedim, typename Number>
NeighborType
BoundingBox<spacedim, Number>::get_neighbor_type(
  const BoundingBox<spacedim, Number> &other_bbox,
  const double                         tolerance) const
{
  if (!has_overlap_with(other_bbox, tolerance))
    return NeighborType::not_neighbors;

  if (spacedim == 1)
    {
      // In dimension 1 if the two bounding box are neighbors
      // we can merge them
      return NeighborType::mergeable_neighbors;
    }
  else
    {
      const std::array<Point<spacedim, Number>, 2> bbox1 = {
        {this->get_boundary_points().first,
         this->get_boundary_points().second}};
      const std::array<Point<spacedim, Number>, 2> bbox2 = {
        {other_bbox.get_boundary_points().first,
         other_bbox.get_boundary_points().second}};

      // The boxes intersect: we need to understand now how they intersect.
      // We begin by computing the intersection:
      std::array<double, spacedim> intersect_bbox_min;
      std::array<double, spacedim> intersect_bbox_max;
      for (unsigned int d = 0; d < spacedim; ++d)
        {
          intersect_bbox_min[d] = std::max(bbox1[0][d], bbox2[0][d]);
          intersect_bbox_max[d] = std::min(bbox1[1][d], bbox2[1][d]);
        }

      // Finding the intersection's dimension
      int intersect_dim = spacedim;
      for (unsigned int d = 0; d < spacedim; ++d)
        if (std::abs(intersect_bbox_min[d] - intersect_bbox_max[d]) <=
            tolerance * (std::abs(intersect_bbox_min[d]) +
                         std::abs(intersect_bbox_max[d])))
          --intersect_dim;

      if (intersect_dim == 0 || intersect_dim == spacedim - 2)
        return NeighborType::simple_neighbors;

      // Checking the two mergeable cases: first if the boxes are aligned so
      // that they can be merged
      unsigned int not_align_1 = 0, not_align_2 = 0;
      bool         same_direction = true;
      for (unsigned int d = 0; d < spacedim; ++d)
        {
          if (std::abs(bbox2[0][d] - bbox1[0][d]) >
              tolerance * (std::abs(bbox2[0][d]) + std::abs(bbox1[0][d])))
            ++not_align_1;
          if (std::abs(bbox1[1][d] - bbox2[1][d]) >
              tolerance * (std::abs(bbox1[1][d]) + std::abs(bbox2[1][d])))
            ++not_align_2;
          if (not_align_1 != not_align_2)
            {
              same_direction = false;
              break;
            }
        }

      if (not_align_1 <= 1 && not_align_2 <= 1 && same_direction)
        return NeighborType::mergeable_neighbors;

      // Second: one box is contained/equal to the other
      if ((this->point_inside(bbox2[0]) && this->point_inside(bbox2[1])) ||
          (other_bbox.point_inside(bbox1[0], tolerance) &&
           other_bbox.point_inside(bbox1[1], tolerance)))
        return NeighborType::mergeable_neighbors;

      // Degenerate and mergeable cases have been found, it remains:
      return NeighborType::attached_neighbors;
    }
}



template <int spacedim, typename Number>
double
BoundingBox<spacedim, Number>::volume() const
{
  double vol = 1.0;
  for (unsigned int i = 0; i < spacedim; ++i)
    vol *= (this->boundary_points.second[i] - this->boundary_points.first[i]);
  return vol;
}



template <int spacedim, typename Number>
Number
BoundingBox<spacedim, Number>::lower_bound(const unsigned int direction) const
{
  AssertIndexRange(direction, spacedim);

  return boundary_points.first[direction];
}



template <int spacedim, typename Number>
Number
BoundingBox<spacedim, Number>::upper_bound(const unsigned int direction) const
{
  AssertIndexRange(direction, spacedim);

  return boundary_points.second[direction];
}



template <int spacedim, typename Number>
Point<spacedim, Number>
BoundingBox<spacedim, Number>::center() const
{
  Point<spacedim, Number> point;
  for (unsigned int i = 0; i < spacedim; ++i)
    point[i] = .5 * (boundary_points.first[i] + boundary_points.second[i]);

  return point;
}



template <int spacedim, typename Number>
BoundingBox<1, Number>
BoundingBox<spacedim, Number>::bounds(const unsigned int direction) const
{
  AssertIndexRange(direction, spacedim);

  std::pair<Point<1, Number>, Point<1, Number>> lower_upper_bounds;
  lower_upper_bounds.first[0]  = lower_bound(direction);
  lower_upper_bounds.second[0] = upper_bound(direction);

  return BoundingBox<1, Number>(lower_upper_bounds);
}



template <int spacedim, typename Number>
Number
BoundingBox<spacedim, Number>::side_length(const unsigned int direction) const
{
  AssertIndexRange(direction, spacedim);

  return boundary_points.second[direction] - boundary_points.first[direction];
}



template <int spacedim, typename Number>
Point<spacedim, Number>
BoundingBox<spacedim, Number>::vertex(const unsigned int index) const
{
  AssertIndexRange(index, GeometryInfo<spacedim>::vertices_per_cell);

  const Point<spacedim> unit_cell_vertex =
    GeometryInfo<spacedim>::unit_cell_vertex(index);

  Point<spacedim, Number> point;
  for (unsigned int i = 0; i < spacedim; ++i)
    point[i] = boundary_points.first[i] + side_length(i) * unit_cell_vertex[i];

  return point;
}



template <int spacedim, typename Number>
BoundingBox<spacedim, Number>
BoundingBox<spacedim, Number>::child(const unsigned int index) const
{
  AssertIndexRange(index, GeometryInfo<spacedim>::max_children_per_cell);

  // Vertex closest to child.
  const Point<spacedim, Number> parent_vertex = vertex(index);
  const Point<spacedim, Number> parent_center = center();

  const Point<spacedim> upper_corner_unit_cell =
    GeometryInfo<spacedim>::unit_cell_vertex(
      GeometryInfo<spacedim>::vertices_per_cell - 1);

  const Point<spacedim> lower_corner_unit_cell =
    GeometryInfo<spacedim>::unit_cell_vertex(0);

  std::pair<Point<spacedim, Number>, Point<spacedim, Number>>
    child_lower_upper_corner;
  for (unsigned int i = 0; i < spacedim; ++i)
    {
      const double child_side_length = side_length(i) / 2;

      const double child_center = (parent_center[i] + parent_vertex[i]) / 2;

      child_lower_upper_corner.first[i] =
        child_center + child_side_length * (lower_corner_unit_cell[i] - .5);
      child_lower_upper_corner.second[i] =
        child_center + child_side_length * (upper_corner_unit_cell[i] - .5);
    }

  return BoundingBox<spacedim, Number>(child_lower_upper_corner);
}



template <int spacedim, typename Number>
BoundingBox<spacedim - 1, Number>
BoundingBox<spacedim, Number>::cross_section(const unsigned int direction) const
{
  AssertIndexRange(direction, spacedim);

  std::pair<Point<spacedim - 1, Number>, Point<spacedim - 1, Number>>
    cross_section_lower_upper_corner;
  for (unsigned int d = 0; d < spacedim - 1; ++d)
    {
      const int index_to_write_from =
        internal::coordinate_to_one_dim_higher<spacedim - 1>(direction, d);

      cross_section_lower_upper_corner.first[d] =
        boundary_points.first[index_to_write_from];

      cross_section_lower_upper_corner.second[d] =
        boundary_points.second[index_to_write_from];
    }

  return BoundingBox<spacedim - 1, Number>(cross_section_lower_upper_corner);
}



template <int spacedim, typename Number>
Point<spacedim, Number>
BoundingBox<spacedim, Number>::real_to_unit(
  const Point<spacedim, Number> &point) const
{
  auto       unit = point;
  const auto diag = boundary_points.second - boundary_points.first;
  unit -= boundary_points.first;
  for (unsigned int d = 0; d < spacedim; ++d)
    unit[d] /= diag[d];
  return unit;
}



template <int spacedim, typename Number>
Point<spacedim, Number>
BoundingBox<spacedim, Number>::unit_to_real(
  const Point<spacedim, Number> &point) const
{
  auto       real = boundary_points.first;
  const auto diag = boundary_points.second - boundary_points.first;
  for (unsigned int d = 0; d < spacedim; ++d)
    real[d] += diag[d] * point[d];
  return real;
}



template <int spacedim, typename Number>
Number
BoundingBox<spacedim, Number>::signed_distance(
  const Point<spacedim, Number> &point,
  const unsigned int             direction) const
{
  const Number p1 = lower_bound(direction);
  const Number p2 = upper_bound(direction);

  if (point[direction] > p2)
    return point[direction] - p2;
  else if (point[direction] < p1)
    return p1 - point[direction];
  else
    return -std::min(point[direction] - p1, p2 - point[direction]);
}



template <int spacedim, typename Number>
Number
BoundingBox<spacedim, Number>::signed_distance(
  const Point<spacedim, Number> &point) const
{
  // calculate vector of orthogonal signed distances
  std::array<Number, spacedim> distances;
  for (unsigned int d = 0; d < spacedim; ++d)
    distances[d] = signed_distance(point, d);

  // determine the number of positive signed distances
  const unsigned int n_positive_signed_distances =
    std::count_if(distances.begin(), distances.end(), [](const auto &a) {
      return a > 0.0;
    });

  if (n_positive_signed_distances <= 1)
    // point is inside of bounding box (0: all signed distances are
    // negative; find the index with the smallest absolute value)
    // or next to a face (1: all signed distances are negative
    // but one; find this index)
    return *std::max_element(distances.begin(), distances.end());
  else
    // point is next to a corner (2D/3D: all signed distances are
    // positive) or a line (3D: all signed distances are positive
    // but one) -> take the l2-norm of all positive signed distances
    return std::sqrt(std::accumulate(distances.begin(),
                                     distances.end(),
                                     0.0,
                                     [](const auto &a, const auto &b) {
                                       return a + (b > 0 ? b * b : 0.0);
                                     }));
}



template <int dim, typename Number>
BoundingBox<dim, Number>
create_unit_bounding_box()
{
  std::pair<Point<dim, Number>, Point<dim, Number>> lower_upper_corner;
  for (unsigned int i = 0; i < dim; ++i)
    {
      lower_upper_corner.second[i] = 1;
    }
  return BoundingBox<dim, Number>(lower_upper_corner);
}


#include "base/bounding_box.inst"
DEAL_II_NAMESPACE_CLOSE
