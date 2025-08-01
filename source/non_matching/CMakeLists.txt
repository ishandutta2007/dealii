## ------------------------------------------------------------------------
##
## SPDX-License-Identifier: LGPL-2.1-or-later
## Copyright (C) 2012 - 2025 by the deal.II authors
##
## This file is part of the deal.II library.
##
## Part of the source code is dual licensed under Apache-2.0 WITH
## LLVM-exception OR LGPL-2.1-or-later. Detailed license information
## governing the source code and code contributions can be found in
## LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
##
## ------------------------------------------------------------------------

set(_src
  fe_immersed_values.cc
  fe_values.cc
  mesh_classifier.cc
  quadrature_generator.cc
  coupling.cc
  immersed_surface_quadrature.cc
  closest_surface_point.cc
  )

  set(_inst
  fe_immersed_values.inst.in
  fe_values.inst.in
  mesh_classifier.inst.in
  quadrature_generator.inst.in
  coupling.inst.in
  closest_surface_point.inst.in
  )

file(GLOB _header CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/include/deal.II/non_matching/*.h
  )

define_object_library(object_non_matching OBJECT ${_src} ${_header} ${_inst})
expand_instantiations(object_non_matching "${_inst}")

collect_source_and_header_files("${_src}" "${_header}")
