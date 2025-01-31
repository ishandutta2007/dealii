// ---------------------------------------------------------------------
//
// Copyright (C) 2017 - 2021 by the deal.II authors
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


// Test that we can form unusual mixed complex-complex / complex-real
// tensor products:

#include <deal.II/base/symmetric_tensor.h>
#include <deal.II/base/tensor.h>

#include "../tests.h"

void
test_symmetric()
{
  constexpr int dim = 3;

  using rank0_complex = std::complex<double>;
  dealii::Tensor<1, dim, rank0_complex>          curl_complex;
  dealii::SymmetricTensor<2, dim, rank0_complex> permeability_complex;

  using rank0_complex_float = std::complex<float>;
  dealii::Tensor<1, dim, rank0_complex_float> curl_complex_float;
  dealii::Tensor<2, dim, rank0_complex_float> permeability_complex_float;

  using rank0 = double;
  dealii::Tensor<1, dim, rank0> curl;
  dealii::Tensor<2, dim, rank0> permeability;

  using rank0_float = double;
  dealii::Tensor<1, dim, rank0_float> curl_float;
  dealii::Tensor<2, dim, rank0_float> permeability_float;

  // clang-format off
  curl_complex * curl;
  curl_complex * curl_complex;
  curl_complex * curl_complex_float;
  curl_complex * curl_float;
  curl_complex_float * curl;
  curl_complex_float * curl_complex;
  curl_complex_float * curl_complex_float;
  curl_complex_float * curl_float;
  curl_complex_float * permeability_complex * curl;
  curl_complex_float * permeability_complex * curl_complex;
  curl_complex_float * permeability_complex * curl_complex_float;
  curl_complex_float * permeability_complex * curl_float;
  curl_complex_float * permeability_complex_float * curl;
  curl_complex_float * permeability_complex_float * curl_complex;
  curl_complex_float * permeability_complex_float * curl_complex_float;
  curl_complex_float * permeability_complex_float * curl_float;
  curl_complex_float * permeability * curl;
  curl_complex_float * permeability * curl_complex;
  curl_complex_float * permeability * curl_complex_float;
  curl_complex_float * permeability * curl_float;
  curl_complex_float * permeability_float * curl;
  curl_complex_float * permeability_float * curl_complex;
  curl_complex * permeability_complex * curl;
  curl_complex * permeability_complex * curl_complex;
  curl_complex * permeability_complex * curl_complex_float;
  curl_complex * permeability_complex * curl_float;
  curl_complex * permeability_complex_float * curl;
  curl_complex * permeability_complex_float * curl_complex;
  curl_complex * permeability_complex_float * curl_complex_float;
  curl_complex * permeability_complex_float * curl_float;
  curl_complex * permeability * curl;
  curl_complex * permeability * curl_complex;
  curl_complex * permeability * curl_complex_float;
  curl_complex * permeability * curl_float;
  curl_complex * permeability_float * curl;
  curl_complex * permeability_float * curl_complex;
  curl * curl;
  curl * curl_complex;
  curl * curl_complex_float;
  curl * curl_float;
  curl_float * curl;
  curl_float * curl_complex;
  curl_float * curl_complex_float;
  curl_float * curl_float;
  curl_float * permeability_complex * curl;
  curl_float * permeability_complex * curl_complex;
  curl_float * permeability_complex * curl_complex_float;
  curl_float * permeability_complex * curl_float;
  curl_float * permeability_complex_float * curl;
  curl_float * permeability_complex_float * curl_complex;
  curl_float * permeability_complex_float * curl_complex_float;
  curl_float * permeability_complex_float * curl_float;
  curl_float * permeability * curl;
  curl_float * permeability * curl_complex;
  curl_float * permeability * curl_complex_float;
  curl_float * permeability * curl_float;
  curl_float * permeability_float * curl;
  curl_float * permeability_float * curl_complex;
  curl * permeability_complex * curl;
  curl * permeability_complex * curl_complex;
  curl * permeability_complex * curl_complex_float;
  curl * permeability_complex * curl_float;
  curl * permeability_complex_float * curl;
  curl * permeability_complex_float * curl_complex;
  curl * permeability_complex_float * curl_complex_float;
  curl * permeability_complex_float * curl_float;
  curl * permeability *  curl;
  curl * permeability *  curl_complex;
  curl * permeability *  curl_complex_float;
  curl * permeability *  curl_float;
  curl * permeability_float *  curl;
  curl * permeability_float *   curl_complex;
  permeability_complex * curl;
  permeability_complex * curl_complex;
  permeability_complex * curl_complex_float;
  permeability_complex * curl_float;
  permeability_complex_float * curl;
  permeability_complex_float * curl_complex;
  permeability_complex_float * curl_complex_float;
  permeability_complex_float * curl_float;
  permeability * curl;
  permeability * curl_complex;
  permeability * curl_complex_float;
  permeability * curl_float;
  permeability_float * curl;
  permeability_float * curl_complex;
  // clang-format on
}

template <int dim>
void
test()
{
  using rank0_complex = std::complex<double>;
  dealii::Tensor<1, dim == 2 ? 1 : dim, rank0_complex> curl_complex;
  dealii::Tensor<dim == 2 ? 0 : 2, dim, rank0_complex> permeability_complex;

  using rank0_complex_float = std::complex<float>;
  dealii::Tensor<1, dim == 2 ? 1 : dim, rank0_complex_float> curl_complex_float;
  dealii::Tensor<dim == 2 ? 0 : 2, dim, rank0_complex_float>
    permeability_complex_float;

  using rank0 = double;
  dealii::Tensor<1, dim == 2 ? 1 : dim, rank0> curl;
  dealii::Tensor<dim == 2 ? 0 : 2, dim, rank0> permeability;

  using rank0_float = double;
  dealii::Tensor<1, dim == 2 ? 1 : dim, rank0_float> curl_float;
  dealii::Tensor<dim == 2 ? 0 : 2, dim, rank0_float> permeability_float;

  // clang-format off
  curl_complex * curl;
  curl_complex * curl_complex;
  curl_complex * curl_complex_float;
  curl_complex * curl_float;
  curl_complex_float * curl;
  curl_complex_float * curl_complex;
  curl_complex_float * curl_complex_float;
  curl_complex_float * curl_float;
  curl_complex_float * permeability_complex * curl;
  curl_complex_float * permeability_complex * curl_complex;
  curl_complex_float * permeability_complex * curl_complex_float;
  curl_complex_float * permeability_complex * curl_float;
  curl_complex_float * permeability_complex_float * curl;
  curl_complex_float * permeability_complex_float * curl_complex;
  curl_complex_float * permeability_complex_float * curl_complex_float;
  curl_complex_float * permeability_complex_float * curl_float;
  curl_complex_float * permeability * curl;
  curl_complex_float * permeability * curl_complex;
  curl_complex_float * permeability * curl_complex_float;
  curl_complex_float * permeability * curl_float;
  curl_complex_float * permeability_float * curl;
  curl_complex_float * permeability_float * curl_complex;
  curl_complex * permeability_complex * curl;
  curl_complex * permeability_complex * curl_complex;
  curl_complex * permeability_complex * curl_complex_float;
  curl_complex * permeability_complex * curl_float;
  curl_complex * permeability_complex_float * curl;
  curl_complex * permeability_complex_float * curl_complex;
  curl_complex * permeability_complex_float * curl_complex_float;
  curl_complex * permeability_complex_float * curl_float;
  curl_complex * permeability * curl;
  curl_complex * permeability * curl_complex;
  curl_complex * permeability * curl_complex_float;
  curl_complex * permeability * curl_float;
  curl_complex * permeability_float * curl;
  curl_complex * permeability_float * curl_complex;
  curl * curl;
  curl * curl_complex;
  curl * curl_complex_float;
  curl * curl_float;
  curl_float * curl;
  curl_float * curl_complex;
  curl_float * curl_complex_float;
  curl_float * curl_float;
  curl_float * permeability_complex * curl;
  curl_float * permeability_complex * curl_complex;
  curl_float * permeability_complex * curl_complex_float;
  curl_float * permeability_complex * curl_float;
  curl_float * permeability_complex_float * curl;
  curl_float * permeability_complex_float * curl_complex;
  curl_float * permeability_complex_float * curl_complex_float;
  curl_float * permeability_complex_float * curl_float;
  curl_float * permeability * curl;
  curl_float * permeability * curl_complex;
  curl_float * permeability * curl_complex_float;
  curl_float * permeability * curl_float;
  curl_float * permeability_float * curl;
  curl_float * permeability_float * curl_complex;
  curl * permeability_complex * curl;
  curl * permeability_complex * curl_complex;
  curl * permeability_complex * curl_complex_float;
  curl * permeability_complex * curl_float;
  curl * permeability_complex_float * curl;
  curl * permeability_complex_float * curl_complex;
  curl * permeability_complex_float * curl_complex_float;
  curl * permeability_complex_float * curl_float;
  curl * permeability * curl;
  curl * permeability * curl_complex;
  curl * permeability * curl_complex_float;
  curl * permeability * curl_float;
  curl * permeability_float * curl;
  curl * permeability_float *  curl_complex;
  permeability_complex * curl;
  permeability_complex * curl_complex;
  permeability_complex * curl_complex_float;
  permeability_complex * curl_float;
  permeability_complex_float * curl;
  permeability_complex_float * curl_complex;
  permeability_complex_float * curl_complex_float;
  permeability_complex_float * curl_float;
  permeability * curl;
  permeability * curl_complex;
  permeability * curl_complex_float;
  permeability * curl_float;
  permeability_float * curl;
  permeability_float * curl_complex;
  // clang-format on
}

int
main(int argc, char *argv[])
{
  initlog();

  test<2>();
  test<3>();
  test_symmetric();

  deallog << "OK" << std::endl;

  return 0;
}
