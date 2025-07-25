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

#include <deal.II/base/function_bessel.h>
#include <deal.II/base/function_lib.h>
#include <deal.II/base/numbers.h>
#include <deal.II/base/point.h>
#include <deal.II/base/std_cxx17/cmath.h>
#include <deal.II/base/tensor.h>

#include <deal.II/lac/vector.h>

#include <cmath>

DEAL_II_NAMESPACE_OPEN


namespace Functions
{
  template <int dim>
  double
  SquareFunction<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    return p.square();
  }


  template <int dim>
  void
  SquareFunction<dim>::vector_value(const Point<dim> &p,
                                    Vector<double>   &values) const
  {
    AssertDimension(values.size(), 1);
    values(0) = p.square();
  }


  template <int dim>
  void
  SquareFunction<dim>::value_list(const std::vector<Point<dim>> &points,
                                  std::vector<double>           &values,
                                  const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        values[i]           = p.square();
      }
  }


  template <int dim>
  double
  SquareFunction<dim>::laplacian(const Point<dim> &, const unsigned int) const
  {
    return 2 * dim;
  }


  template <int dim>
  void
  SquareFunction<dim>::laplacian_list(const std::vector<Point<dim>> &points,
                                      std::vector<double>           &values,
                                      const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = 2 * dim;
  }



  template <int dim>
  Tensor<1, dim>
  SquareFunction<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    return p * 2.;
  }


  template <int dim>
  void
  SquareFunction<dim>::vector_gradient(
    const Point<dim>            &p,
    std::vector<Tensor<1, dim>> &values) const
  {
    AssertDimension(values.size(), 1);
    values[0] = p * 2.;
  }



  template <int dim>
  void
  SquareFunction<dim>::gradient_list(const std::vector<Point<dim>> &points,
                                     std::vector<Tensor<1, dim>>   &gradients,
                                     const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      gradients[i] = static_cast<Tensor<1, dim>>(points[i]) * 2;
  }


  //--------------------------------------------------------------------


  template <int dim>
  double
  Q1WedgeFunction<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    Assert(dim >= 2, ExcInternalError());
    return p[0] * p[1];
  }



  template <int dim>
  void
  Q1WedgeFunction<dim>::value_list(const std::vector<Point<dim>> &points,
                                   std::vector<double>           &values,
                                   const unsigned int) const
  {
    Assert(dim >= 2, ExcInternalError());
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        values[i]           = p[0] * p[1];
      }
  }


  template <int dim>
  void
  Q1WedgeFunction<dim>::vector_value_list(
    const std::vector<Point<dim>> &points,
    std::vector<Vector<double>>   &values) const
  {
    Assert(dim >= 2, ExcInternalError());
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));
    Assert(values[0].size() == 1, ExcDimensionMismatch(values[0].size(), 1));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        values[i](0)        = p[0] * p[1];
      }
  }


  template <int dim>
  double
  Q1WedgeFunction<dim>::laplacian(const Point<dim> &, const unsigned int) const
  {
    Assert(dim >= 2, ExcInternalError());
    return 0.;
  }


  template <int dim>
  void
  Q1WedgeFunction<dim>::laplacian_list(const std::vector<Point<dim>> &points,
                                       std::vector<double>           &values,
                                       const unsigned int) const
  {
    Assert(dim >= 2, ExcInternalError());
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = 0.;
  }



  template <int dim>
  Tensor<1, dim>
  Q1WedgeFunction<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    Assert(dim >= 2, ExcInternalError());
    Tensor<1, dim> erg;
    erg[0] = p[1];
    erg[1] = p[0];
    return erg;
  }



  template <int dim>
  void
  Q1WedgeFunction<dim>::gradient_list(const std::vector<Point<dim>> &points,
                                      std::vector<Tensor<1, dim>>   &gradients,
                                      const unsigned int) const
  {
    Assert(dim >= 2, ExcInternalError());
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        gradients[i][0] = points[i][1];
        gradients[i][1] = points[i][0];
      }
  }


  template <int dim>
  void
  Q1WedgeFunction<dim>::vector_gradient_list(
    const std::vector<Point<dim>>            &points,
    std::vector<std::vector<Tensor<1, dim>>> &gradients) const
  {
    Assert(dim >= 2, ExcInternalError());
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));
    Assert(gradients[0].size() == 1,
           ExcDimensionMismatch(gradients[0].size(), 1));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        gradients[i][0][0] = points[i][1];
        gradients[i][0][1] = points[i][0];
      }
  }


  //--------------------------------------------------------------------


  template <int dim>
  PillowFunction<dim>::PillowFunction(const double offset)
    : offset(offset)
  {}


  template <int dim>
  double
  PillowFunction<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    switch (dim)
      {
        case 1:
          return 1. - p[0] * p[0] + offset;
        case 2:
          return (1. - p[0] * p[0]) * (1. - p[1] * p[1]) + offset;
        case 3:
          return (1. - p[0] * p[0]) * (1. - p[1] * p[1]) * (1. - p[2] * p[2]) +
                 offset;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }

  template <int dim>
  void
  PillowFunction<dim>::value_list(const std::vector<Point<dim>> &points,
                                  std::vector<double>           &values,
                                  const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              values[i] = 1. - p[0] * p[0] + offset;
              break;
            case 2:
              values[i] = (1. - p[0] * p[0]) * (1. - p[1] * p[1]) + offset;
              break;
            case 3:
              values[i] =
                (1. - p[0] * p[0]) * (1. - p[1] * p[1]) * (1. - p[2] * p[2]) +
                offset;
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }



  template <int dim>
  double
  PillowFunction<dim>::laplacian(const Point<dim> &p, const unsigned int) const
  {
    switch (dim)
      {
        case 1:
          return -2.;
        case 2:
          return -2. * ((1. - p[0] * p[0]) + (1. - p[1] * p[1]));
        case 3:
          return -2. * ((1. - p[0] * p[0]) * (1. - p[1] * p[1]) +
                        (1. - p[1] * p[1]) * (1. - p[2] * p[2]) +
                        (1. - p[2] * p[2]) * (1. - p[0] * p[0]));
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }

  template <int dim>
  void
  PillowFunction<dim>::laplacian_list(const std::vector<Point<dim>> &points,
                                      std::vector<double>           &values,
                                      const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              values[i] = -2.;
              break;
            case 2:
              values[i] = -2. * ((1. - p[0] * p[0]) + (1. - p[1] * p[1]));
              break;
            case 3:
              values[i] = -2. * ((1. - p[0] * p[0]) * (1. - p[1] * p[1]) +
                                 (1. - p[1] * p[1]) * (1. - p[2] * p[2]) +
                                 (1. - p[2] * p[2]) * (1. - p[0] * p[0]));
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  template <int dim>
  Tensor<1, dim>
  PillowFunction<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    Tensor<1, dim> result;
    switch (dim)
      {
        case 1:
          result[0] = -2. * p[0];
          break;
        case 2:
          result[0] = -2. * p[0] * (1. - p[1] * p[1]);
          result[1] = -2. * p[1] * (1. - p[0] * p[0]);
          break;
        case 3:
          result[0] = -2. * p[0] * (1. - p[1] * p[1]) * (1. - p[2] * p[2]);
          result[1] = -2. * p[1] * (1. - p[0] * p[0]) * (1. - p[2] * p[2]);
          result[2] = -2. * p[2] * (1. - p[0] * p[0]) * (1. - p[1] * p[1]);
          break;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return result;
  }

  template <int dim>
  void
  PillowFunction<dim>::gradient_list(const std::vector<Point<dim>> &points,
                                     std::vector<Tensor<1, dim>>   &gradients,
                                     const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              gradients[i][0] = -2. * p[0];
              break;
            case 2:
              gradients[i][0] = -2. * p[0] * (1. - p[1] * p[1]);
              gradients[i][1] = -2. * p[1] * (1. - p[0] * p[0]);
              break;
            case 3:
              gradients[i][0] =
                -2. * p[0] * (1. - p[1] * p[1]) * (1. - p[2] * p[2]);
              gradients[i][1] =
                -2. * p[1] * (1. - p[0] * p[0]) * (1. - p[2] * p[2]);
              gradients[i][2] =
                -2. * p[2] * (1. - p[0] * p[0]) * (1. - p[1] * p[1]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  //--------------------------------------------------------------------

  template <int dim>
  CosineFunction<dim>::CosineFunction(const unsigned int n_components)
    : Function<dim>(n_components)
  {}



  template <int dim>
  double
  CosineFunction<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    switch (dim)
      {
        case 1:
          return std::cos(numbers::PI_2 * p[0]);
        case 2:
          return std::cos(numbers::PI_2 * p[0]) *
                 std::cos(numbers::PI_2 * p[1]);
        case 3:
          return std::cos(numbers::PI_2 * p[0]) *
                 std::cos(numbers::PI_2 * p[1]) *
                 std::cos(numbers::PI_2 * p[2]);
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }

  template <int dim>
  void
  CosineFunction<dim>::value_list(const std::vector<Point<dim>> &points,
                                  std::vector<double>           &values,
                                  const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = value(points[i]);
  }


  template <int dim>
  void
  CosineFunction<dim>::vector_value_list(
    const std::vector<Point<dim>> &points,
    std::vector<Vector<double>>   &values) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const double v = value(points[i]);
        for (unsigned int k = 0; k < values[i].size(); ++k)
          values[i](k) = v;
      }
  }


  template <int dim>
  double
  CosineFunction<dim>::laplacian(const Point<dim> &p, const unsigned int) const
  {
    switch (dim)
      {
        case 1:
          return -numbers::PI_2 * numbers::PI_2 *
                 std::cos(numbers::PI_2 * p[0]);
        case 2:
          return -2 * numbers::PI_2 * numbers::PI_2 *
                 std::cos(numbers::PI_2 * p[0]) *
                 std::cos(numbers::PI_2 * p[1]);
        case 3:
          return -3 * numbers::PI_2 * numbers::PI_2 *
                 std::cos(numbers::PI_2 * p[0]) *
                 std::cos(numbers::PI_2 * p[1]) *
                 std::cos(numbers::PI_2 * p[2]);
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }

  template <int dim>
  void
  CosineFunction<dim>::laplacian_list(const std::vector<Point<dim>> &points,
                                      std::vector<double>           &values,
                                      const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = laplacian(points[i]);
  }

  template <int dim>
  Tensor<1, dim>
  CosineFunction<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    Tensor<1, dim> result;
    switch (dim)
      {
        case 1:
          result[0] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]);
          break;
        case 2:
          result[0] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                      std::cos(numbers::PI_2 * p[1]);
          result[1] = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                      std::sin(numbers::PI_2 * p[1]);
          break;
        case 3:
          result[0] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                      std::cos(numbers::PI_2 * p[1]) *
                      std::cos(numbers::PI_2 * p[2]);
          result[1] = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                      std::sin(numbers::PI_2 * p[1]) *
                      std::cos(numbers::PI_2 * p[2]);
          result[2] = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                      std::cos(numbers::PI_2 * p[1]) *
                      std::sin(numbers::PI_2 * p[2]);
          break;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return result;
  }

  template <int dim>
  void
  CosineFunction<dim>::gradient_list(const std::vector<Point<dim>> &points,
                                     std::vector<Tensor<1, dim>>   &gradients,
                                     const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              gradients[i][0] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]);
              break;
            case 2:
              gradients[i][0] = -numbers::PI_2 *
                                std::sin(numbers::PI_2 * p[0]) *
                                std::cos(numbers::PI_2 * p[1]);
              gradients[i][1] = -numbers::PI_2 *
                                std::cos(numbers::PI_2 * p[0]) *
                                std::sin(numbers::PI_2 * p[1]);
              break;
            case 3:
              gradients[i][0] =
                -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                std::cos(numbers::PI_2 * p[1]) * std::cos(numbers::PI_2 * p[2]);
              gradients[i][1] =
                -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                std::sin(numbers::PI_2 * p[1]) * std::cos(numbers::PI_2 * p[2]);
              gradients[i][2] =
                -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                std::cos(numbers::PI_2 * p[1]) * std::sin(numbers::PI_2 * p[2]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  template <int dim>
  SymmetricTensor<2, dim>
  CosineFunction<dim>::hessian(const Point<dim> &p, const unsigned int) const
  {
    const double pi2 = numbers::PI_2 * numbers::PI_2;

    SymmetricTensor<2, dim> result;
    switch (dim)
      {
        case 1:
          result[0][0] = -pi2 * std::cos(numbers::PI_2 * p[0]);
          break;
        case 2:
          {
            const double coco = -pi2 * std::cos(numbers::PI_2 * p[0]) *
                                std::cos(numbers::PI_2 * p[1]);
            const double sisi = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                std::sin(numbers::PI_2 * p[1]);
            result[0][0] = coco;
            result[1][1] = coco;
            // for SymmetricTensor we assign [ij] and [ji] simultaneously:
            result[0][1] = sisi;
          }
          break;
        case 3:
          {
            const double cococo = -pi2 * std::cos(numbers::PI_2 * p[0]) *
                                  std::cos(numbers::PI_2 * p[1]) *
                                  std::cos(numbers::PI_2 * p[2]);
            const double sisico = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                  std::sin(numbers::PI_2 * p[1]) *
                                  std::cos(numbers::PI_2 * p[2]);
            const double sicosi = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                  std::cos(numbers::PI_2 * p[1]) *
                                  std::sin(numbers::PI_2 * p[2]);
            const double cosisi = pi2 * std::cos(numbers::PI_2 * p[0]) *
                                  std::sin(numbers::PI_2 * p[1]) *
                                  std::sin(numbers::PI_2 * p[2]);

            result[0][0] = cococo;
            result[1][1] = cococo;
            result[2][2] = cococo;
            // for SymmetricTensor we assign [ij] and [ji] simultaneously:
            result[0][1] = sisico;
            result[0][2] = sicosi;
            result[1][2] = cosisi;
          }
          break;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return result;
  }

  template <int dim>
  void
  CosineFunction<dim>::hessian_list(
    const std::vector<Point<dim>>        &points,
    std::vector<SymmetricTensor<2, dim>> &hessians,
    const unsigned int) const
  {
    Assert(hessians.size() == points.size(),
           ExcDimensionMismatch(hessians.size(), points.size()));

    const double pi2 = numbers::PI_2 * numbers::PI_2;

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              hessians[i][0][0] = -pi2 * std::cos(numbers::PI_2 * p[0]);
              break;
            case 2:
              {
                const double coco = -pi2 * std::cos(numbers::PI_2 * p[0]) *
                                    std::cos(numbers::PI_2 * p[1]);
                const double sisi = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                    std::sin(numbers::PI_2 * p[1]);
                hessians[i][0][0] = coco;
                hessians[i][1][1] = coco;
                // for SymmetricTensor we assign [ij] and [ji] simultaneously:
                hessians[i][0][1] = sisi;
              }
              break;
            case 3:
              {
                const double cococo = -pi2 * std::cos(numbers::PI_2 * p[0]) *
                                      std::cos(numbers::PI_2 * p[1]) *
                                      std::cos(numbers::PI_2 * p[2]);
                const double sisico = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                      std::sin(numbers::PI_2 * p[1]) *
                                      std::cos(numbers::PI_2 * p[2]);
                const double sicosi = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                      std::cos(numbers::PI_2 * p[1]) *
                                      std::sin(numbers::PI_2 * p[2]);
                const double cosisi = pi2 * std::cos(numbers::PI_2 * p[0]) *
                                      std::sin(numbers::PI_2 * p[1]) *
                                      std::sin(numbers::PI_2 * p[2]);

                hessians[i][0][0] = cococo;
                hessians[i][1][1] = cococo;
                hessians[i][2][2] = cococo;
                // for SymmetricTensor we assign [ij] and [ji] simultaneously:
                hessians[i][0][1] = sisico;
                hessians[i][0][2] = sicosi;
                hessians[i][1][2] = cosisi;
              }
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  //--------------------------------------------------------------------

  template <int dim>
  CosineGradFunction<dim>::CosineGradFunction()
    : Function<dim>(dim)
  {}


  template <int dim>
  double
  CosineGradFunction<dim>::value(const Point<dim>  &p,
                                 const unsigned int d) const
  {
    AssertIndexRange(d, dim);
    const unsigned int d1 = (d + 1) % dim;
    const unsigned int d2 = (d + 2) % dim;
    switch (dim)
      {
        case 1:
          return (-numbers::PI_2 * std::sin(numbers::PI_2 * p[0]));
        case 2:
          return (-numbers::PI_2 * std::sin(numbers::PI_2 * p[d]) *
                  std::cos(numbers::PI_2 * p[d1]));
        case 3:
          return (-numbers::PI_2 * std::sin(numbers::PI_2 * p[d]) *
                  std::cos(numbers::PI_2 * p[d1]) *
                  std::cos(numbers::PI_2 * p[d2]));
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }


  template <int dim>
  void
  CosineGradFunction<dim>::vector_value(const Point<dim> &p,
                                        Vector<double>   &result) const
  {
    AssertDimension(result.size(), dim);
    switch (dim)
      {
        case 1:
          result(0) = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]);
          break;
        case 2:
          result(0) = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                      std::cos(numbers::PI_2 * p[1]);
          result(1) = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                      std::sin(numbers::PI_2 * p[1]);
          break;
        case 3:
          result(0) = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                      std::cos(numbers::PI_2 * p[1]) *
                      std::cos(numbers::PI_2 * p[2]);
          result(1) = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                      std::sin(numbers::PI_2 * p[1]) *
                      std::cos(numbers::PI_2 * p[2]);
          result(2) = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                      std::cos(numbers::PI_2 * p[1]) *
                      std::sin(numbers::PI_2 * p[2]);
          break;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
  }


  template <int dim>
  void
  CosineGradFunction<dim>::value_list(const std::vector<Point<dim>> &points,
                                      std::vector<double>           &values,
                                      const unsigned int             d) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));
    AssertIndexRange(d, dim);
    const unsigned int d1 = (d + 1) % dim;
    const unsigned int d2 = (d + 2) % dim;

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              values[i] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[d]);
              break;
            case 2:
              values[i] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[d]) *
                          std::cos(numbers::PI_2 * p[d1]);
              break;
            case 3:
              values[i] = -numbers::PI_2 * std::sin(numbers::PI_2 * p[d]) *
                          std::cos(numbers::PI_2 * p[d1]) *
                          std::cos(numbers::PI_2 * p[d2]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }


  template <int dim>
  void
  CosineGradFunction<dim>::vector_value_list(
    const std::vector<Point<dim>> &points,
    std::vector<Vector<double>>   &values) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              values[i](0) = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]);
              break;
            case 2:
              values[i](0) = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                             std::cos(numbers::PI_2 * p[1]);
              values[i](1) = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                             std::sin(numbers::PI_2 * p[1]);
              break;
            case 3:
              values[i](0) = -numbers::PI_2 * std::sin(numbers::PI_2 * p[0]) *
                             std::cos(numbers::PI_2 * p[1]) *
                             std::cos(numbers::PI_2 * p[2]);
              values[i](1) = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                             std::sin(numbers::PI_2 * p[1]) *
                             std::cos(numbers::PI_2 * p[2]);
              values[i](2) = -numbers::PI_2 * std::cos(numbers::PI_2 * p[0]) *
                             std::cos(numbers::PI_2 * p[1]) *
                             std::sin(numbers::PI_2 * p[2]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }


  template <int dim>
  double
  CosineGradFunction<dim>::laplacian(const Point<dim>  &p,
                                     const unsigned int d) const
  {
    return -numbers::PI_2 * numbers::PI_2 * value(p, d);
  }


  template <int dim>
  Tensor<1, dim>
  CosineGradFunction<dim>::gradient(const Point<dim>  &p,
                                    const unsigned int d) const
  {
    AssertIndexRange(d, dim);
    const unsigned int d1  = (d + 1) % dim;
    const unsigned int d2  = (d + 2) % dim;
    const double       pi2 = numbers::PI_2 * numbers::PI_2;

    Tensor<1, dim> result;
    switch (dim)
      {
        case 1:
          result[0] = -pi2 * std::cos(numbers::PI_2 * p[0]);
          break;
        case 2:
          result[d] = -pi2 * std::cos(numbers::PI_2 * p[d]) *
                      std::cos(numbers::PI_2 * p[d1]);
          result[d1] = pi2 * std::sin(numbers::PI_2 * p[d]) *
                       std::sin(numbers::PI_2 * p[d1]);
          break;
        case 3:
          result[d] = -pi2 * std::cos(numbers::PI_2 * p[d]) *
                      std::cos(numbers::PI_2 * p[d1]) *
                      std::cos(numbers::PI_2 * p[d2]);
          result[d1] = pi2 * std::sin(numbers::PI_2 * p[d]) *
                       std::sin(numbers::PI_2 * p[d1]) *
                       std::cos(numbers::PI_2 * p[d2]);
          result[d2] = pi2 * std::sin(numbers::PI_2 * p[d]) *
                       std::cos(numbers::PI_2 * p[d1]) *
                       std::sin(numbers::PI_2 * p[d2]);
          break;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return result;
  }


  template <int dim>
  void
  CosineGradFunction<dim>::gradient_list(const std::vector<Point<dim>> &points,
                                         std::vector<Tensor<1, dim>> &gradients,
                                         const unsigned int           d) const
  {
    AssertIndexRange(d, dim);
    const unsigned int d1  = (d + 1) % dim;
    const unsigned int d2  = (d + 2) % dim;
    const double       pi2 = numbers::PI_2 * numbers::PI_2;

    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));
    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p      = points[i];
        Tensor<1, dim>   &result = gradients[i];

        switch (dim)
          {
            case 1:
              result[0] = -pi2 * std::cos(numbers::PI_2 * p[0]);
              break;
            case 2:
              result[d] = -pi2 * std::cos(numbers::PI_2 * p[d]) *
                          std::cos(numbers::PI_2 * p[d1]);
              result[d1] = pi2 * std::sin(numbers::PI_2 * p[d]) *
                           std::sin(numbers::PI_2 * p[d1]);
              break;
            case 3:
              result[d] = -pi2 * std::cos(numbers::PI_2 * p[d]) *
                          std::cos(numbers::PI_2 * p[d1]) *
                          std::cos(numbers::PI_2 * p[d2]);
              result[d1] = pi2 * std::sin(numbers::PI_2 * p[d]) *
                           std::sin(numbers::PI_2 * p[d1]) *
                           std::cos(numbers::PI_2 * p[d2]);
              result[d2] = pi2 * std::sin(numbers::PI_2 * p[d]) *
                           std::cos(numbers::PI_2 * p[d1]) *
                           std::sin(numbers::PI_2 * p[d2]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }


  template <int dim>
  void
  CosineGradFunction<dim>::vector_gradient_list(
    const std::vector<Point<dim>>            &points,
    std::vector<std::vector<Tensor<1, dim>>> &gradients) const
  {
    AssertVectorVectorDimension(gradients, points.size(), dim);
    const double pi2 = numbers::PI_2 * numbers::PI_2;

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              gradients[i][0][0] = -pi2 * std::cos(numbers::PI_2 * p[0]);
              break;
            case 2:
              {
                const double coco = -pi2 * std::cos(numbers::PI_2 * p[0]) *
                                    std::cos(numbers::PI_2 * p[1]);
                const double sisi = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                    std::sin(numbers::PI_2 * p[1]);
                gradients[i][0][0] = coco;
                gradients[i][1][1] = coco;
                gradients[i][0][1] = sisi;
                gradients[i][1][0] = sisi;
              }
              break;
            case 3:
              {
                const double cococo = -pi2 * std::cos(numbers::PI_2 * p[0]) *
                                      std::cos(numbers::PI_2 * p[1]) *
                                      std::cos(numbers::PI_2 * p[2]);
                const double sisico = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                      std::sin(numbers::PI_2 * p[1]) *
                                      std::cos(numbers::PI_2 * p[2]);
                const double sicosi = pi2 * std::sin(numbers::PI_2 * p[0]) *
                                      std::cos(numbers::PI_2 * p[1]) *
                                      std::sin(numbers::PI_2 * p[2]);
                const double cosisi = pi2 * std::cos(numbers::PI_2 * p[0]) *
                                      std::sin(numbers::PI_2 * p[1]) *
                                      std::sin(numbers::PI_2 * p[2]);

                gradients[i][0][0] = cococo;
                gradients[i][1][1] = cococo;
                gradients[i][2][2] = cococo;
                gradients[i][0][1] = sisico;
                gradients[i][1][0] = sisico;
                gradients[i][0][2] = sicosi;
                gradients[i][2][0] = sicosi;
                gradients[i][1][2] = cosisi;
                gradients[i][2][1] = cosisi;
              }
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }


  //--------------------------------------------------------------------

  template <int dim>
  double
  ExpFunction<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    switch (dim)
      {
        case 1:
          return std::exp(p[0]);
        case 2:
          return std::exp(p[0]) * std::exp(p[1]);
        case 3:
          return std::exp(p[0]) * std::exp(p[1]) * std::exp(p[2]);
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }

  template <int dim>
  void
  ExpFunction<dim>::value_list(const std::vector<Point<dim>> &points,
                               std::vector<double>           &values,
                               const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              values[i] = std::exp(p[0]);
              break;
            case 2:
              values[i] = std::exp(p[0]) * std::exp(p[1]);
              break;
            case 3:
              values[i] = std::exp(p[0]) * std::exp(p[1]) * std::exp(p[2]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  template <int dim>
  double
  ExpFunction<dim>::laplacian(const Point<dim> &p, const unsigned int) const
  {
    switch (dim)
      {
        case 1:
          return std::exp(p[0]);
        case 2:
          return 2 * std::exp(p[0]) * std::exp(p[1]);
        case 3:
          return 3 * std::exp(p[0]) * std::exp(p[1]) * std::exp(p[2]);
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return 0.;
  }

  template <int dim>
  void
  ExpFunction<dim>::laplacian_list(const std::vector<Point<dim>> &points,
                                   std::vector<double>           &values,
                                   const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              values[i] = std::exp(p[0]);
              break;
            case 2:
              values[i] = 2 * std::exp(p[0]) * std::exp(p[1]);
              break;
            case 3:
              values[i] = 3 * std::exp(p[0]) * std::exp(p[1]) * std::exp(p[2]);
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  template <int dim>
  Tensor<1, dim>
  ExpFunction<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    Tensor<1, dim> result;
    switch (dim)
      {
        case 1:
          result[0] = std::exp(p[0]);
          break;
        case 2:
          result[0] = std::exp(p[0]) * std::exp(p[1]);
          result[1] = result[0];
          break;
        case 3:
          result[0] = std::exp(p[0]) * std::exp(p[1]) * std::exp(p[2]);
          result[1] = result[0];
          result[2] = result[0];
          break;
        default:
          DEAL_II_NOT_IMPLEMENTED();
      }
    return result;
  }

  template <int dim>
  void
  ExpFunction<dim>::gradient_list(const std::vector<Point<dim>> &points,
                                  std::vector<Tensor<1, dim>>   &gradients,
                                  const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p = points[i];
        switch (dim)
          {
            case 1:
              gradients[i][0] = std::exp(p[0]);
              break;
            case 2:
              gradients[i][0] = std::exp(p[0]) * std::exp(p[1]);
              gradients[i][1] = gradients[i][0];
              break;
            case 3:
              gradients[i][0] =
                std::exp(p[0]) * std::exp(p[1]) * std::exp(p[2]);
              gradients[i][1] = gradients[i][0];
              gradients[i][2] = gradients[i][0];
              break;
            default:
              DEAL_II_NOT_IMPLEMENTED();
          }
      }
  }

  //--------------------------------------------------------------------


  double
  LSingularityFunction::value(const Point<2> &p, const unsigned int) const
  {
    const double x = p[0];
    const double y = p[1];

    if ((x >= 0) && (y >= 0))
      return 0.;

    const double phi       = std::atan2(y, -x) + numbers::PI;
    const double r_squared = x * x + y * y;

    return std::cbrt(r_squared) * std::sin(2. / 3. * phi);
  }



  void
  LSingularityFunction::value_list(const std::vector<Point<2>> &points,
                                   std::vector<double>         &values,
                                   const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const double x = points[i][0];
        const double y = points[i][1];

        if ((x >= 0) && (y >= 0))
          values[i] = 0.;
        else
          {
            const double phi       = std::atan2(y, -x) + numbers::PI;
            const double r_squared = x * x + y * y;

            values[i] = std::cbrt(r_squared) * std::sin(2. / 3. * phi);
          }
      }
  }



  void
  LSingularityFunction::vector_value_list(
    const std::vector<Point<2>> &points,
    std::vector<Vector<double>> &values) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        Assert(values[i].size() == 1,
               ExcDimensionMismatch(values[i].size(), 1));
        const double x = points[i][0];
        const double y = points[i][1];

        if ((x >= 0) && (y >= 0))
          values[i](0) = 0.;
        else
          {
            const double phi       = std::atan2(y, -x) + numbers::PI;
            const double r_squared = x * x + y * y;

            values[i](0) = std::cbrt(r_squared) * std::sin(2. / 3. * phi);
          }
      }
  }



  double
  LSingularityFunction::laplacian(const Point<2> &, const unsigned int) const
  {
    // Not a bug but exactly how the function is defined:
    return 0.;
  }



  void
  LSingularityFunction::laplacian_list(const std::vector<Point<2>> &points,
                                       std::vector<double>         &values,
                                       const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = 0.;
  }



  Tensor<1, 2>
  LSingularityFunction::gradient(const Point<2> &p, const unsigned int) const
  {
    const double x   = p[0];
    const double y   = p[1];
    const double phi = std::atan2(y, -x) + numbers::PI;
    const double r43 = std::pow(x * x + y * y, 2. / 3.);

    Tensor<1, 2> result;
    result[0] = 2. / 3. *
                (std::sin(2. / 3. * phi) * x + std::cos(2. / 3. * phi) * y) /
                r43;
    result[1] = 2. / 3. *
                (std::sin(2. / 3. * phi) * y - std::cos(2. / 3. * phi) * x) /
                r43;
    return result;
  }



  void
  LSingularityFunction::gradient_list(const std::vector<Point<2>> &points,
                                      std::vector<Tensor<1, 2>>   &gradients,
                                      const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<2> &p   = points[i];
        const double    x   = p[0];
        const double    y   = p[1];
        const double    phi = std::atan2(y, -x) + numbers::PI;
        const double    r43 = std::pow(x * x + y * y, 2. / 3.);

        gradients[i][0] =
          2. / 3. *
          (std::sin(2. / 3. * phi) * x + std::cos(2. / 3. * phi) * y) / r43;
        gradients[i][1] =
          2. / 3. *
          (std::sin(2. / 3. * phi) * y - std::cos(2. / 3. * phi) * x) / r43;
      }
  }



  void
  LSingularityFunction::vector_gradient_list(
    const std::vector<Point<2>>            &points,
    std::vector<std::vector<Tensor<1, 2>>> &gradients) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        Assert(gradients[i].size() == 1,
               ExcDimensionMismatch(gradients[i].size(), 1));
        const Point<2> &p   = points[i];
        const double    x   = p[0];
        const double    y   = p[1];
        const double    phi = std::atan2(y, -x) + numbers::PI;
        const double    r43 = std::pow(x * x + y * y, 2. / 3.);

        gradients[i][0][0] =
          2. / 3. *
          (std::sin(2. / 3. * phi) * x + std::cos(2. / 3. * phi) * y) / r43;
        gradients[i][0][1] =
          2. / 3. *
          (std::sin(2. / 3. * phi) * y - std::cos(2. / 3. * phi) * x) / r43;
      }
  }

  //--------------------------------------------------------------------

  LSingularityGradFunction::LSingularityGradFunction()
    : Function<2>(2)
  {}



  double
  LSingularityGradFunction::value(const Point<2> &p, const unsigned int d) const
  {
    AssertIndexRange(d, 2);

    const double x   = p[0];
    const double y   = p[1];
    const double phi = std::atan2(y, -x) + numbers::PI;
    const double r43 = std::pow(x * x + y * y, 2. / 3.);

    return 2. / 3. *
           (std::sin(2. / 3. * phi) * p[d] +
            (d == 0 ? (std::cos(2. / 3. * phi) * y) :
                      (-std::cos(2. / 3. * phi) * x))) /
           r43;
  }


  void
  LSingularityGradFunction::value_list(const std::vector<Point<2>> &points,
                                       std::vector<double>         &values,
                                       const unsigned int           d) const
  {
    AssertIndexRange(d, 2);
    AssertDimension(values.size(), points.size());

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<2> &p   = points[i];
        const double    x   = p[0];
        const double    y   = p[1];
        const double    phi = std::atan2(y, -x) + numbers::PI;
        const double    r43 = std::pow(x * x + y * y, 2. / 3.);

        values[i] = 2. / 3. *
                    (std::sin(2. / 3. * phi) * p[d] +
                     (d == 0 ? (std::cos(2. / 3. * phi) * y) :
                               (-std::cos(2. / 3. * phi) * x))) /
                    r43;
      }
  }


  void
  LSingularityGradFunction::vector_value_list(
    const std::vector<Point<2>> &points,
    std::vector<Vector<double>> &values) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        AssertDimension(values[i].size(), 2);
        const Point<2> &p   = points[i];
        const double    x   = p[0];
        const double    y   = p[1];
        const double    phi = std::atan2(y, -x) + numbers::PI;
        const double    r43 = std::pow(x * x + y * y, 2. / 3.);

        values[i](0) =
          2. / 3. *
          (std::sin(2. / 3. * phi) * x + std::cos(2. / 3. * phi) * y) / r43;
        values[i](1) =
          2. / 3. *
          (std::sin(2. / 3. * phi) * y - std::cos(2. / 3. * phi) * x) / r43;
      }
  }


  double
  LSingularityGradFunction::laplacian(const Point<2> &,
                                      const unsigned int) const
  {
    return 0.;
  }


  void
  LSingularityGradFunction::laplacian_list(const std::vector<Point<2>> &points,
                                           std::vector<double>         &values,
                                           const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = 0.;
  }



  Tensor<1, 2>
  LSingularityGradFunction::gradient(const Point<2> & /*p*/,
                                     const unsigned int /*component*/) const
  {
    DEAL_II_NOT_IMPLEMENTED();
    return {};
  }


  void
  LSingularityGradFunction::gradient_list(
    const std::vector<Point<2>> & /*points*/,
    std::vector<Tensor<1, 2>> & /*gradients*/,
    const unsigned int /*component*/) const
  {
    DEAL_II_NOT_IMPLEMENTED();
  }


  void
  LSingularityGradFunction::vector_gradient_list(
    const std::vector<Point<2>> & /*points*/,
    std::vector<std::vector<Tensor<1, 2>>> & /*gradients*/) const
  {
    DEAL_II_NOT_IMPLEMENTED();
  }

  //--------------------------------------------------------------------

  template <int dim>
  double
  SlitSingularityFunction<dim>::value(const Point<dim> &p,
                                      const unsigned int) const
  {
    const double x = p[0];
    const double y = p[1];

    const double phi       = std::atan2(x, y) + numbers::PI;
    const double r_squared = x * x + y * y;

    return std::pow(r_squared, .25) * std::sin(.5 * phi);
  }


  template <int dim>
  void
  SlitSingularityFunction<dim>::value_list(
    const std::vector<Point<dim>> &points,
    std::vector<double>           &values,
    const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const double x = points[i][0];
        const double y = points[i][1];

        const double phi       = std::atan2(x, y) + numbers::PI;
        const double r_squared = x * x + y * y;

        values[i] = std::pow(r_squared, .25) * std::sin(.5 * phi);
      }
  }


  template <int dim>
  void
  SlitSingularityFunction<dim>::vector_value_list(
    const std::vector<Point<dim>> &points,
    std::vector<Vector<double>>   &values) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        Assert(values[i].size() == 1,
               ExcDimensionMismatch(values[i].size(), 1));

        const double x = points[i][0];
        const double y = points[i][1];

        const double phi       = std::atan2(x, y) + numbers::PI;
        const double r_squared = x * x + y * y;

        values[i](0) = std::pow(r_squared, .25) * std::sin(.5 * phi);
      }
  }


  template <int dim>
  double
  SlitSingularityFunction<dim>::laplacian(const Point<dim> &,
                                          const unsigned int) const
  {
    return 0.;
  }


  template <int dim>
  void
  SlitSingularityFunction<dim>::laplacian_list(
    const std::vector<Point<dim>> &points,
    std::vector<double>           &values,
    const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = 0.;
  }


  template <int dim>
  Tensor<1, dim>
  SlitSingularityFunction<dim>::gradient(const Point<dim> &p,
                                         const unsigned int) const
  {
    const double x   = p[0];
    const double y   = p[1];
    const double phi = std::atan2(x, y) + numbers::PI;
    const double r64 = std::pow(x * x + y * y, 3. / 4.);

    Tensor<1, dim> result;
    result[0] = 1. / 2. *
                (std::sin(1. / 2. * phi) * x + std::cos(1. / 2. * phi) * y) /
                r64;
    result[1] = 1. / 2. *
                (std::sin(1. / 2. * phi) * y - std::cos(1. / 2. * phi) * x) /
                r64;
    return result;
  }


  template <int dim>
  void
  SlitSingularityFunction<dim>::gradient_list(
    const std::vector<Point<dim>> &points,
    std::vector<Tensor<1, dim>>   &gradients,
    const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<dim> &p   = points[i];
        const double      x   = p[0];
        const double      y   = p[1];
        const double      phi = std::atan2(x, y) + numbers::PI;
        const double      r64 = std::pow(x * x + y * y, 3. / 4.);

        gradients[i][0] =
          1. / 2. *
          (std::sin(1. / 2. * phi) * x + std::cos(1. / 2. * phi) * y) / r64;
        gradients[i][1] =
          1. / 2. *
          (std::sin(1. / 2. * phi) * y - std::cos(1. / 2. * phi) * x) / r64;
        for (unsigned int d = 2; d < dim; ++d)
          gradients[i][d] = 0.;
      }
  }

  template <int dim>
  void
  SlitSingularityFunction<dim>::vector_gradient_list(
    const std::vector<Point<dim>>            &points,
    std::vector<std::vector<Tensor<1, dim>>> &gradients) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        Assert(gradients[i].size() == 1,
               ExcDimensionMismatch(gradients[i].size(), 1));

        const Point<dim> &p   = points[i];
        const double      x   = p[0];
        const double      y   = p[1];
        const double      phi = std::atan2(x, y) + numbers::PI;
        const double      r64 = std::pow(x * x + y * y, 3. / 4.);

        gradients[i][0][0] =
          1. / 2. *
          (std::sin(1. / 2. * phi) * x + std::cos(1. / 2. * phi) * y) / r64;
        gradients[i][0][1] =
          1. / 2. *
          (std::sin(1. / 2. * phi) * y - std::cos(1. / 2. * phi) * x) / r64;
        for (unsigned int d = 2; d < dim; ++d)
          gradients[i][0][d] = 0.;
      }
  }

  //--------------------------------------------------------------------


  double
  SlitHyperSingularityFunction::value(const Point<2> &p,
                                      const unsigned int) const
  {
    const double x = p[0];
    const double y = p[1];

    const double phi       = std::atan2(x, y) + numbers::PI;
    const double r_squared = x * x + y * y;

    return std::pow(r_squared, .125) * std::sin(.25 * phi);
  }


  void
  SlitHyperSingularityFunction::value_list(const std::vector<Point<2>> &points,
                                           std::vector<double>         &values,
                                           const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const double x = points[i][0];
        const double y = points[i][1];

        const double phi       = std::atan2(x, y) + numbers::PI;
        const double r_squared = x * x + y * y;

        values[i] = std::pow(r_squared, .125) * std::sin(.25 * phi);
      }
  }


  void
  SlitHyperSingularityFunction::vector_value_list(
    const std::vector<Point<2>> &points,
    std::vector<Vector<double>> &values) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        Assert(values[i].size() == 1,
               ExcDimensionMismatch(values[i].size(), 1));

        const double x = points[i][0];
        const double y = points[i][1];

        const double phi       = std::atan2(x, y) + numbers::PI;
        const double r_squared = x * x + y * y;

        values[i](0) = std::pow(r_squared, .125) * std::sin(.25 * phi);
      }
  }


  double
  SlitHyperSingularityFunction::laplacian(const Point<2> &,
                                          const unsigned int) const
  {
    return 0.;
  }


  void
  SlitHyperSingularityFunction::laplacian_list(
    const std::vector<Point<2>> &points,
    std::vector<double>         &values,
    const unsigned int) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = 0.;
  }


  Tensor<1, 2>
  SlitHyperSingularityFunction::gradient(const Point<2> &p,
                                         const unsigned int) const
  {
    const double x   = p[0];
    const double y   = p[1];
    const double phi = std::atan2(x, y) + numbers::PI;
    const double r78 = std::pow(x * x + y * y, 7. / 8.);


    Tensor<1, 2> result;
    result[0] = 1. / 4. *
                (std::sin(1. / 4. * phi) * x + std::cos(1. / 4. * phi) * y) /
                r78;
    result[1] = 1. / 4. *
                (std::sin(1. / 4. * phi) * y - std::cos(1. / 4. * phi) * x) /
                r78;
    return result;
  }


  void
  SlitHyperSingularityFunction::gradient_list(
    const std::vector<Point<2>> &points,
    std::vector<Tensor<1, 2>>   &gradients,
    const unsigned int) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        const Point<2> &p   = points[i];
        const double    x   = p[0];
        const double    y   = p[1];
        const double    phi = std::atan2(x, y) + numbers::PI;
        const double    r78 = std::pow(x * x + y * y, 7. / 8.);

        gradients[i][0] =
          1. / 4. *
          (std::sin(1. / 4. * phi) * x + std::cos(1. / 4. * phi) * y) / r78;
        gradients[i][1] =
          1. / 4. *
          (std::sin(1. / 4. * phi) * y - std::cos(1. / 4. * phi) * x) / r78;
      }
  }


  void
  SlitHyperSingularityFunction::vector_gradient_list(
    const std::vector<Point<2>>            &points,
    std::vector<std::vector<Tensor<1, 2>>> &gradients) const
  {
    Assert(gradients.size() == points.size(),
           ExcDimensionMismatch(gradients.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      {
        Assert(gradients[i].size() == 1,
               ExcDimensionMismatch(gradients[i].size(), 1));

        const Point<2> &p   = points[i];
        const double    x   = p[0];
        const double    y   = p[1];
        const double    phi = std::atan2(x, y) + numbers::PI;
        const double    r78 = std::pow(x * x + y * y, 7. / 8.);

        gradients[i][0][0] =
          1. / 4. *
          (std::sin(1. / 4. * phi) * x + std::cos(1. / 4. * phi) * y) / r78;
        gradients[i][0][1] =
          1. / 4. *
          (std::sin(1. / 4. * phi) * y - std::cos(1. / 4. * phi) * x) / r78;
      }
  }

  //--------------------------------------------------------------------

  template <int dim>
  JumpFunction<dim>::JumpFunction(const Point<dim> &direction,
                                  const double      steepness)
    : direction(direction)
    , steepness(steepness)
  {
    switch (dim)
      {
        case 1:
          angle = 0;
          break;
        case 2:
          angle = std::atan2(direction[0], direction[1]);
          break;
        default:
          angle = std::numeric_limits<double>::signaling_NaN();
          DEAL_II_NOT_IMPLEMENTED();
      }
    sine   = std::sin(angle);
    cosine = std::cos(angle);
  }



  template <int dim>
  double
  JumpFunction<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    const double x = steepness * (-cosine * p[0] + sine * p[1]);
    return -std::atan(x);
  }



  template <int dim>
  void
  JumpFunction<dim>::value_list(const std::vector<Point<dim>> &p,
                                std::vector<double>           &values,
                                const unsigned int) const
  {
    Assert(values.size() == p.size(),
           ExcDimensionMismatch(values.size(), p.size()));

    for (unsigned int i = 0; i < p.size(); ++i)
      {
        const double x = steepness * (-cosine * p[i][0] + sine * p[i][1]);
        values[i]      = -std::atan(x);
      }
  }


  template <int dim>
  double
  JumpFunction<dim>::laplacian(const Point<dim> &p, const unsigned int) const
  {
    const double x = steepness * (-cosine * p[0] + sine * p[1]);
    const double r = 1 + x * x;
    return 2 * steepness * steepness * x / (r * r);
  }


  template <int dim>
  void
  JumpFunction<dim>::laplacian_list(const std::vector<Point<dim>> &p,
                                    std::vector<double>           &values,
                                    const unsigned int) const
  {
    Assert(values.size() == p.size(),
           ExcDimensionMismatch(values.size(), p.size()));

    double f = 2 * steepness * steepness;

    for (unsigned int i = 0; i < p.size(); ++i)
      {
        const double x = steepness * (-cosine * p[i][0] + sine * p[i][1]);
        const double r = 1 + x * x;
        values[i]      = f * x / (r * r);
      }
  }



  template <int dim>
  Tensor<1, dim>
  JumpFunction<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    const double   x = steepness * (-cosine * p[0] + sine * p[1]);
    const double   r = -steepness * (1 + x * x);
    Tensor<1, dim> erg;
    erg[0] = cosine * r;
    erg[1] = sine * r;
    return erg;
  }



  template <int dim>
  void
  JumpFunction<dim>::gradient_list(const std::vector<Point<dim>> &p,
                                   std::vector<Tensor<1, dim>>   &gradients,
                                   const unsigned int) const
  {
    Assert(gradients.size() == p.size(),
           ExcDimensionMismatch(gradients.size(), p.size()));

    for (unsigned int i = 0; i < p.size(); ++i)
      {
        const double x  = steepness * (cosine * p[i][0] + sine * p[i][1]);
        const double r  = -steepness * (1 + x * x);
        gradients[i][0] = cosine * r;
        gradients[i][1] = sine * r;
      }
  }



  template <int dim>
  std::size_t
  JumpFunction<dim>::memory_consumption() const
  {
    // only simple data elements, so
    // use sizeof operator
    return sizeof(*this);
  }



  /* ---------------------- FourierCosineFunction ----------------------- */


  template <int dim>
  FourierCosineFunction<dim>::FourierCosineFunction(
    const Tensor<1, dim> &fourier_coefficients)
    : Function<dim>(1)
    , fourier_coefficients(fourier_coefficients)
  {}



  template <int dim>
  double
  FourierCosineFunction<dim>::value(const Point<dim>  &p,
                                    const unsigned int component) const
  {
    AssertIndexRange(component, 1);
    return std::cos(fourier_coefficients * p);
  }



  template <int dim>
  Tensor<1, dim>
  FourierCosineFunction<dim>::gradient(const Point<dim>  &p,
                                       const unsigned int component) const
  {
    AssertIndexRange(component, 1);
    return -fourier_coefficients * std::sin(fourier_coefficients * p);
  }



  template <int dim>
  double
  FourierCosineFunction<dim>::laplacian(const Point<dim>  &p,
                                        const unsigned int component) const
  {
    AssertIndexRange(component, 1);
    return (fourier_coefficients * fourier_coefficients) *
           (-std::cos(fourier_coefficients * p));
  }



  /* ---------------------- FourierSineFunction ----------------------- */



  template <int dim>
  FourierSineFunction<dim>::FourierSineFunction(
    const Tensor<1, dim> &fourier_coefficients)
    : Function<dim>(1)
    , fourier_coefficients(fourier_coefficients)
  {}



  template <int dim>
  double
  FourierSineFunction<dim>::value(const Point<dim>  &p,
                                  const unsigned int component) const
  {
    AssertIndexRange(component, 1);
    return std::sin(fourier_coefficients * p);
  }



  template <int dim>
  Tensor<1, dim>
  FourierSineFunction<dim>::gradient(const Point<dim>  &p,
                                     const unsigned int component) const
  {
    AssertIndexRange(component, 1);
    return fourier_coefficients * std::cos(fourier_coefficients * p);
  }



  template <int dim>
  double
  FourierSineFunction<dim>::laplacian(const Point<dim>  &p,
                                      const unsigned int component) const
  {
    AssertIndexRange(component, 1);
    return (fourier_coefficients * fourier_coefficients) *
           (-std::sin(fourier_coefficients * p));
  }



  /* ---------------------- FourierSineSum ----------------------- */



  template <int dim>
  FourierSineSum<dim>::FourierSineSum(
    const std::vector<Point<dim>> &fourier_coefficients,
    const std::vector<double>     &weights)
    : Function<dim>(1)
    , fourier_coefficients(fourier_coefficients)
    , weights(weights)
  {
    Assert(fourier_coefficients.size() > 0, ExcZero());
    Assert(fourier_coefficients.size() == weights.size(),
           ExcDimensionMismatch(fourier_coefficients.size(), weights.size()));
  }



  template <int dim>
  double
  FourierSineSum<dim>::value(const Point<dim>  &p,
                             const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    const unsigned int n   = weights.size();
    double             sum = 0;
    for (unsigned int s = 0; s < n; ++s)
      sum += weights[s] * std::sin(fourier_coefficients[s] * p);

    return sum;
  }



  template <int dim>
  Tensor<1, dim>
  FourierSineSum<dim>::gradient(const Point<dim>  &p,
                                const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    const unsigned int n = weights.size();
    Tensor<1, dim>     sum;
    for (unsigned int s = 0; s < n; ++s)
      sum += fourier_coefficients[s] * std::cos(fourier_coefficients[s] * p);

    return sum;
  }



  template <int dim>
  double
  FourierSineSum<dim>::laplacian(const Point<dim>  &p,
                                 const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    const unsigned int n   = weights.size();
    double             sum = 0;
    for (unsigned int s = 0; s < n; ++s)
      sum -= (fourier_coefficients[s] * fourier_coefficients[s]) *
             std::sin(fourier_coefficients[s] * p);

    return sum;
  }



  /* ---------------------- FourierCosineSum ----------------------- */



  template <int dim>
  FourierCosineSum<dim>::FourierCosineSum(
    const std::vector<Point<dim>> &fourier_coefficients,
    const std::vector<double>     &weights)
    : Function<dim>(1)
    , fourier_coefficients(fourier_coefficients)
    , weights(weights)
  {
    Assert(fourier_coefficients.size() > 0, ExcZero());
    Assert(fourier_coefficients.size() == weights.size(),
           ExcDimensionMismatch(fourier_coefficients.size(), weights.size()));
  }



  template <int dim>
  double
  FourierCosineSum<dim>::value(const Point<dim>  &p,
                               const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    const unsigned int n   = weights.size();
    double             sum = 0;
    for (unsigned int s = 0; s < n; ++s)
      sum += weights[s] * std::cos(fourier_coefficients[s] * p);

    return sum;
  }



  template <int dim>
  Tensor<1, dim>
  FourierCosineSum<dim>::gradient(const Point<dim>  &p,
                                  const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    const unsigned int n = weights.size();
    Tensor<1, dim>     sum;
    for (unsigned int s = 0; s < n; ++s)
      sum -= fourier_coefficients[s] * std::sin(fourier_coefficients[s] * p);

    return sum;
  }



  template <int dim>
  double
  FourierCosineSum<dim>::laplacian(const Point<dim>  &p,
                                   const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    const unsigned int n   = weights.size();
    double             sum = 0;
    for (unsigned int s = 0; s < n; ++s)
      sum -= (fourier_coefficients[s] * fourier_coefficients[s]) *
             std::cos(fourier_coefficients[s] * p);

    return sum;
  }



  /* ---------------------- Monomial ----------------------- */



  template <int dim, typename Number>
  Monomial<dim, Number>::Monomial(const Tensor<1, dim, Number> &exponents,
                                  const unsigned int            n_components)
    : Function<dim, Number>(n_components)
    , exponents(exponents)
  {}



  template <int dim, typename Number>
  Number
  Monomial<dim, Number>::value(const Point<dim>  &p,
                               const unsigned int component) const
  {
    AssertIndexRange(component, this->n_components);

    Number prod = 1;
    for (unsigned int s = 0; s < dim; ++s)
      {
        if (p[s] < 0)
          Assert(std::floor(exponents[s]) == exponents[s],
                 ExcMessage("Exponentiation of a negative base number with "
                            "a real exponent can't be performed."));
        prod *= std::pow(p[s], exponents[s]);
      }
    return prod;
  }



  template <int dim, typename Number>
  void
  Monomial<dim, Number>::vector_value(const Point<dim> &p,
                                      Vector<Number>   &values) const
  {
    Assert(values.size() == this->n_components,
           ExcDimensionMismatch(values.size(), this->n_components));

    for (unsigned int i = 0; i < values.size(); ++i)
      values(i) = Monomial<dim, Number>::value(p, i);
  }



  template <int dim, typename Number>
  Tensor<1, dim, Number>
  Monomial<dim, Number>::gradient(const Point<dim>  &p,
                                  const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    Tensor<1, dim> r;
    for (unsigned int d = 0; d < dim; ++d)
      {
        double prod = 1;
        for (unsigned int s = 0; s < dim; ++s)
          {
            if ((s == d) && (exponents[s] == 0) && (p[s] == 0))
              {
                prod = 0;
                break;
              }
            else
              {
                if (p[s] < 0)
                  Assert(std::floor(exponents[s]) == exponents[s],
                         ExcMessage(
                           "Exponentiation of a negative base number with "
                           "a real exponent can't be performed."));
                prod *=
                  (s == d ? exponents[s] * std::pow(p[s], exponents[s] - 1) :
                            std::pow(p[s], exponents[s]));
              }
          }
        r[d] = prod;
      }

    return r;
  }



  template <int dim, typename Number>
  void
  Monomial<dim, Number>::value_list(const std::vector<Point<dim>> &points,
                                    std::vector<Number>           &values,
                                    const unsigned int component) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = Monomial<dim, Number>::value(points[i], component);
  }


  template <int dim>
  Bessel1<dim>::Bessel1(const unsigned int order,
                        const double       wave_number,
                        const Point<dim>   center)
    : order(order)
    , wave_number(wave_number)
    , center(center)
  {
    Assert(wave_number >= 0., ExcMessage("wave_number must be nonnegative!"));
  }

  template <int dim>
  double
  Bessel1<dim>::value(const Point<dim> &p, const unsigned int) const
  {
    Assert(dim == 2, ExcNotImplemented());
    const double r = p.distance(center);
    return std_cxx17::cyl_bessel_j(order, r * wave_number);
  }


  template <int dim>
  void
  Bessel1<dim>::value_list(const std::vector<Point<dim>> &points,
                           std::vector<double>           &values,
                           const unsigned int) const
  {
    Assert(dim == 2, ExcNotImplemented());
    AssertDimension(points.size(), values.size());
    for (unsigned int k = 0; k < points.size(); ++k)
      {
        const double r = points[k].distance(center);
        values[k]      = std_cxx17::cyl_bessel_j(order, r * wave_number);
      }
  }


  template <int dim>
  Tensor<1, dim>
  Bessel1<dim>::gradient(const Point<dim> &p, const unsigned int) const
  {
    Assert(dim == 2, ExcNotImplemented());
    const double r  = p.distance(center);
    const double co = (r == 0.) ? 0. : (p[0] - center[0]) / r;
    const double si = (r == 0.) ? 0. : (p[1] - center[1]) / r;

    const double dJn =
      (order == 0) ?
        (-std_cxx17::cyl_bessel_j(1, r * wave_number)) :
        (.5 * (std_cxx17::cyl_bessel_j(order - 1, wave_number * r) -
               std_cxx17::cyl_bessel_j(order + 1, wave_number * r)));
    Tensor<1, dim> result;
    result[0] = wave_number * co * dJn;
    result[1] = wave_number * si * dJn;
    return result;
  }



  template <int dim>
  void
  Bessel1<dim>::gradient_list(const std::vector<Point<dim>> &points,
                              std::vector<Tensor<1, dim>>   &gradients,
                              const unsigned int) const
  {
    Assert(dim == 2, ExcNotImplemented());
    AssertDimension(points.size(), gradients.size());
    for (unsigned int k = 0; k < points.size(); ++k)
      {
        const Point<dim> &p  = points[k];
        const double      r  = p.distance(center);
        const double      co = (r == 0.) ? 0. : (p[0] - center[0]) / r;
        const double      si = (r == 0.) ? 0. : (p[1] - center[1]) / r;

        const double dJn =
          (order == 0) ?
            (-std_cxx17::cyl_bessel_j(1, r * wave_number)) :
            (.5 * (std_cxx17::cyl_bessel_j(order - 1, wave_number * r) -
                   std_cxx17::cyl_bessel_j(order + 1, wave_number * r)));
        Tensor<1, dim> &result = gradients[k];
        result[0]              = wave_number * co * dJn;
        result[1]              = wave_number * si * dJn;
      }
  }



  namespace
  {
    // interpolate a data value from a table where ix denotes
    // the (lower) left endpoint of the interval to interpolate
    // in, and p_unit denotes the point in unit coordinates to do so.
    double
    interpolate(const Table<1, double> &data_values,
                const TableIndices<1>  &ix,
                const Point<1>         &xi)
    {
      return ((1 - xi[0]) * data_values[ix[0]] +
              xi[0] * data_values[ix[0] + 1]);
    }

    double
    interpolate(const Table<2, double> &data_values,
                const TableIndices<2>  &ix,
                const Point<2>         &p_unit)
    {
      return (((1 - p_unit[0]) * data_values[ix[0]][ix[1]] +
               p_unit[0] * data_values[ix[0] + 1][ix[1]]) *
                (1 - p_unit[1]) +
              ((1 - p_unit[0]) * data_values[ix[0]][ix[1] + 1] +
               p_unit[0] * data_values[ix[0] + 1][ix[1] + 1]) *
                p_unit[1]);
    }

    double
    interpolate(const Table<3, double> &data_values,
                const TableIndices<3>  &ix,
                const Point<3>         &p_unit)
    {
      return ((((1 - p_unit[0]) * data_values[ix[0]][ix[1]][ix[2]] +
                p_unit[0] * data_values[ix[0] + 1][ix[1]][ix[2]]) *
                 (1 - p_unit[1]) +
               ((1 - p_unit[0]) * data_values[ix[0]][ix[1] + 1][ix[2]] +
                p_unit[0] * data_values[ix[0] + 1][ix[1] + 1][ix[2]]) *
                 p_unit[1]) *
                (1 - p_unit[2]) +
              (((1 - p_unit[0]) * data_values[ix[0]][ix[1]][ix[2] + 1] +
                p_unit[0] * data_values[ix[0] + 1][ix[1]][ix[2] + 1]) *
                 (1 - p_unit[1]) +
               ((1 - p_unit[0]) * data_values[ix[0]][ix[1] + 1][ix[2] + 1] +
                p_unit[0] * data_values[ix[0] + 1][ix[1] + 1][ix[2] + 1]) *
                 p_unit[1]) *
                p_unit[2]);
    }


    // Interpolate the gradient of a data value from a table where ix
    // denotes the lower left endpoint of the interval to interpolate
    // in, p_unit denotes the point in unit coordinates, and dx
    // denotes the width of the interval in each dimension.
    Tensor<1, 1>
    gradient_interpolate(const Table<1, double> &data_values,
                         const TableIndices<1>  &ix,
                         const Point<1> & /*p_unit*/,
                         const Point<1> &dx)
    {
      Tensor<1, 1> grad;
      grad[0] = (data_values[ix[0] + 1] - data_values[ix[0]]) / dx[0];
      return grad;
    }


    Tensor<1, 2>
    gradient_interpolate(const Table<2, double> &data_values,
                         const TableIndices<2>  &ix,
                         const Point<2>         &p_unit,
                         const Point<2>         &dx)
    {
      Tensor<1, 2> grad;
      double       u00 = data_values[ix[0]][ix[1]],
             u01       = data_values[ix[0] + 1][ix[1]],
             u10       = data_values[ix[0]][ix[1] + 1],
             u11       = data_values[ix[0] + 1][ix[1] + 1];

      grad[0] =
        ((1 - p_unit[1]) * (u01 - u00) + p_unit[1] * (u11 - u10)) / dx[0];
      grad[1] =
        ((1 - p_unit[0]) * (u10 - u00) + p_unit[0] * (u11 - u01)) / dx[1];
      return grad;
    }


    Tensor<1, 3>
    gradient_interpolate(const Table<3, double> &data_values,
                         const TableIndices<3>  &ix,
                         const Point<3>         &p_unit,
                         const Point<3>         &dx)
    {
      Tensor<1, 3> grad;
      double       u000 = data_values[ix[0]][ix[1]][ix[2]],
             u001       = data_values[ix[0] + 1][ix[1]][ix[2]],
             u010       = data_values[ix[0]][ix[1] + 1][ix[2]],
             u100       = data_values[ix[0]][ix[1]][ix[2] + 1],
             u011       = data_values[ix[0] + 1][ix[1] + 1][ix[2]],
             u101       = data_values[ix[0] + 1][ix[1]][ix[2] + 1],
             u110       = data_values[ix[0]][ix[1] + 1][ix[2] + 1],
             u111       = data_values[ix[0] + 1][ix[1] + 1][ix[2] + 1];

      grad[0] =
        ((1 - p_unit[2]) *
           ((1 - p_unit[1]) * (u001 - u000) + p_unit[1] * (u011 - u010)) +
         p_unit[2] *
           ((1 - p_unit[1]) * (u101 - u100) + p_unit[1] * (u111 - u110))) /
        dx[0];
      grad[1] =
        ((1 - p_unit[2]) *
           ((1 - p_unit[0]) * (u010 - u000) + p_unit[0] * (u011 - u001)) +
         p_unit[2] *
           ((1 - p_unit[0]) * (u110 - u100) + p_unit[0] * (u111 - u101))) /
        dx[1];
      grad[2] =
        ((1 - p_unit[1]) *
           ((1 - p_unit[0]) * (u100 - u000) + p_unit[0] * (u101 - u001)) +
         p_unit[1] *
           ((1 - p_unit[0]) * (u110 - u010) + p_unit[0] * (u111 - u011))) /
        dx[2];

      return grad;
    }
  } // namespace



  template <int dim>
  InterpolatedTensorProductGridData<dim>::InterpolatedTensorProductGridData(
    const std::array<std::vector<double>, dim> &coordinate_values,
    const Table<dim, double>                   &data_values)
    : coordinate_values(coordinate_values)
    , data_values(data_values)
  {
    for (unsigned int d = 0; d < dim; ++d)
      {
        Assert(
          coordinate_values[d].size() >= 2,
          ExcMessage(
            "Coordinate arrays must have at least two coordinate values!"));
        for (unsigned int i = 0; i < coordinate_values[d].size() - 1; ++i)
          Assert(
            coordinate_values[d][i] < coordinate_values[d][i + 1],
            ExcMessage(
              "Coordinate arrays must be sorted in strictly ascending order."));

        Assert(data_values.size()[d] == coordinate_values[d].size(),
               ExcMessage(
                 "Data and coordinate tables do not have the same size."));
      }
  }



  template <int dim>
  InterpolatedTensorProductGridData<dim>::InterpolatedTensorProductGridData(
    std::array<std::vector<double>, dim> &&coordinate_values,
    Table<dim, double>                   &&data_values)
    : coordinate_values(std::move(coordinate_values))
    , data_values(std::move(data_values))
  {
    for (unsigned int d = 0; d < dim; ++d)
      {
        Assert(
          this->coordinate_values[d].size() >= 2,
          ExcMessage(
            "Coordinate arrays must have at least two coordinate values!"));
        for (unsigned int i = 0; i < this->coordinate_values[d].size() - 1; ++i)
          Assert(
            this->coordinate_values[d][i] < this->coordinate_values[d][i + 1],
            ExcMessage(
              "Coordinate arrays must be sorted in strictly ascending order."));

        Assert(this->data_values.size()[d] == this->coordinate_values[d].size(),
               ExcMessage(
                 "Data and coordinate tables do not have the same size."));
      }
  }



  template <int dim>
  TableIndices<dim>
  InterpolatedTensorProductGridData<dim>::table_index_of_point(
    const Point<dim> &p) const
  {
    // find out where this data point lies, relative to the given
    // points. if we run all the way to the end of the range,
    // set the indices so that we will simply query the last of the
    // intervals, starting at x.size()-2 and going to x.size()-1.
    TableIndices<dim> ix;
    for (unsigned int d = 0; d < dim; ++d)
      {
        // get the index of the first element of the coordinate arrays that is
        // larger than p[d]
        ix[d] = (std::lower_bound(coordinate_values[d].begin(),
                                  coordinate_values[d].end(),
                                  p[d]) -
                 coordinate_values[d].begin());

        // the one we want is the index of the coordinate to the left, however,
        // so decrease it by one (unless we have a point to the left of all, in
        // which case we stay where we are; the formulas below are made in a way
        // that allow us to extend the function by a constant value)
        //
        // to make this work, if we got coordinate_values[d].end(), we actually
        // have to consider the last box which has index size()-2
        if (ix[d] == coordinate_values[d].size())
          ix[d] = coordinate_values[d].size() - 2;
        else if (ix[d] > 0)
          --ix[d];
      }

    return ix;
  }



  template <int dim>
  std::size_t
  InterpolatedTensorProductGridData<dim>::memory_consumption() const
  {
    return sizeof(*this) +
           MemoryConsumption::memory_consumption(coordinate_values) -
           sizeof(coordinate_values) +
           MemoryConsumption::memory_consumption(data_values) -
           sizeof(data_values);
  }



  template <int dim>
  const Table<dim, double> &
  InterpolatedTensorProductGridData<dim>::get_data() const
  {
    return data_values;
  }



  template <int dim>
  double
  InterpolatedTensorProductGridData<dim>::value(
    const Point<dim>  &p,
    const unsigned int component) const
  {
    Assert(
      component == 0,
      ExcMessage(
        "This is a scalar function object, the component can only be zero."));

    // find the index in the data table of the cell containing the input point
    const TableIndices<dim> ix = table_index_of_point(p);

    // now compute the relative point within the interval/rectangle/box
    // defined by the point coordinates found above. truncate below and
    // above to accommodate points that may lie outside the range
    Point<dim> p_unit;
    for (unsigned int d = 0; d < dim; ++d)
      p_unit[d] = std::clamp((p[d] - coordinate_values[d][ix[d]]) /
                               (coordinate_values[d][ix[d] + 1] -
                                coordinate_values[d][ix[d]]),
                             0.,
                             1.);

    return interpolate(data_values, ix, p_unit);
  }



  template <int dim>
  Tensor<1, dim>
  InterpolatedTensorProductGridData<dim>::gradient(
    const Point<dim>  &p,
    const unsigned int component) const
  {
    Assert(
      component == 0,
      ExcMessage(
        "This is a scalar function object, the component can only be zero."));

    // find out where this data point lies
    const TableIndices<dim> ix = table_index_of_point(p);

    Point<dim> dx;
    for (unsigned int d = 0; d < dim; ++d)
      dx[d] = coordinate_values[d][ix[d] + 1] - coordinate_values[d][ix[d]];

    Point<dim> p_unit;
    for (unsigned int d = 0; d < dim; ++d)
      p_unit[d] =
        std::clamp((p[d] - coordinate_values[d][ix[d]]) / dx[d], 0., 1.);

    return gradient_interpolate(data_values, ix, p_unit, dx);
  }



  template <int dim>
  InterpolatedUniformGridData<dim>::InterpolatedUniformGridData(
    const std::array<std::pair<double, double>, dim> &interval_endpoints,
    const std::array<unsigned int, dim>              &n_subintervals,
    const Table<dim, double>                         &data_values)
    : interval_endpoints(interval_endpoints)
    , n_subintervals(n_subintervals)
    , data_values(data_values)
  {
    for (unsigned int d = 0; d < dim; ++d)
      {
        Assert(n_subintervals[d] >= 1,
               ExcMessage("There needs to be at least one subinterval in each "
                          "coordinate direction."));
        Assert(interval_endpoints[d].first < interval_endpoints[d].second,
               ExcMessage("The interval in each coordinate direction needs "
                          "to have positive size"));
        Assert(data_values.size()[d] == n_subintervals[d] + 1,
               ExcMessage("The data table does not have the correct size."));
      }
  }



  template <int dim>
  InterpolatedUniformGridData<dim>::InterpolatedUniformGridData(
    std::array<std::pair<double, double>, dim> &&interval_endpoints,
    std::array<unsigned int, dim>              &&n_subintervals,
    Table<dim, double>                         &&data_values)
    : interval_endpoints(std::move(interval_endpoints))
    , n_subintervals(std::move(n_subintervals))
    , data_values(std::move(data_values))
  {
    for (unsigned int d = 0; d < dim; ++d)
      {
        Assert(this->n_subintervals[d] >= 1,
               ExcMessage("There needs to be at least one subinterval in each "
                          "coordinate direction."));
        Assert(this->interval_endpoints[d].first <
                 this->interval_endpoints[d].second,
               ExcMessage("The interval in each coordinate direction needs "
                          "to have positive size"));
        Assert(this->data_values.size()[d] == this->n_subintervals[d] + 1,
               ExcMessage("The data table does not have the correct size."));
      }
  }



  template <int dim>
  double
  InterpolatedUniformGridData<dim>::value(const Point<dim>  &p,
                                          const unsigned int component) const
  {
    Assert(
      component == 0,
      ExcMessage(
        "This is a scalar function object, the component can only be zero."));

    // find out where this data point lies, relative to the given
    // subdivision points
    TableIndices<dim> ix;
    for (unsigned int d = 0; d < dim; ++d)
      {
        const double delta_x =
          ((interval_endpoints[d].second - interval_endpoints[d].first) /
           n_subintervals[d]);
        if (p[d] <= interval_endpoints[d].first)
          ix[d] = 0;
        else if (p[d] >= interval_endpoints[d].second - delta_x)
          ix[d] = n_subintervals[d] - 1;
        else
          ix[d] = static_cast<unsigned int>(
            (p[d] - interval_endpoints[d].first) / delta_x);
      }

    // now compute the relative point within the interval/rectangle/box
    // defined by the point coordinates found above. truncate below and
    // above to accommodate points that may lie outside the range
    Point<dim> p_unit;
    for (unsigned int d = 0; d < dim; ++d)
      {
        const double delta_x =
          ((interval_endpoints[d].second - interval_endpoints[d].first) /
           n_subintervals[d]);

        p_unit[d] =
          std::clamp((p[d] - interval_endpoints[d].first - ix[d] * delta_x) /
                       delta_x,
                     0.,
                     1.);
      }

    return interpolate(data_values, ix, p_unit);
  }



  template <int dim>
  Tensor<1, dim>
  InterpolatedUniformGridData<dim>::gradient(const Point<dim>  &p,
                                             const unsigned int component) const
  {
    Assert(
      component == 0,
      ExcMessage(
        "This is a scalar function object, the component can only be zero."));

    // find out where this data point lies, relative to the given
    // subdivision points
    TableIndices<dim> ix;
    for (unsigned int d = 0; d < dim; ++d)
      {
        const double delta_x = ((this->interval_endpoints[d].second -
                                 this->interval_endpoints[d].first) /
                                this->n_subintervals[d]);
        if (p[d] <= this->interval_endpoints[d].first)
          ix[d] = 0;
        else if (p[d] >= this->interval_endpoints[d].second - delta_x)
          ix[d] = this->n_subintervals[d] - 1;
        else
          ix[d] = static_cast<unsigned int>(
            (p[d] - this->interval_endpoints[d].first) / delta_x);
      }

    // now compute the relative point within the interval/rectangle/box
    // defined by the point coordinates found above. truncate below and
    // above to accommodate points that may lie outside the range
    Point<dim> p_unit;
    Point<dim> delta_x;
    for (unsigned int d = 0; d < dim; ++d)
      {
        delta_x[d] = ((this->interval_endpoints[d].second -
                       this->interval_endpoints[d].first) /
                      this->n_subintervals[d]);
        p_unit[d]  = std::clamp((p[d] - this->interval_endpoints[d].first -
                                ix[d] * delta_x[d]) /
                                 delta_x[d],
                               0.,
                               1.);
      }

    return gradient_interpolate(this->data_values, ix, p_unit, delta_x);
  }



  template <int dim>
  std::size_t
  InterpolatedUniformGridData<dim>::memory_consumption() const
  {
    return sizeof(*this) + data_values.memory_consumption() -
           sizeof(data_values);
  }



  template <int dim>
  const Table<dim, double> &
  InterpolatedUniformGridData<dim>::get_data() const
  {
    return data_values;
  }



  /* ---------------------- Polynomial ----------------------- */



  template <int dim>
  Polynomial<dim>::Polynomial(const Table<2, double>    &exponents,
                              const std::vector<double> &coefficients)
    : Function<dim>(1)
    , exponents(exponents)
    , coefficients(coefficients)
  {
    Assert(exponents.n_rows() == coefficients.size(),
           ExcDimensionMismatch(exponents.n_rows(), coefficients.size()));
    Assert(exponents.n_cols() == dim,
           ExcDimensionMismatch(exponents.n_cols(), dim));
  }



  template <int dim>
  double
  Polynomial<dim>::value(const Point<dim>  &p,
                         const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    double sum = 0;
    for (unsigned int monom = 0; monom < exponents.n_rows(); ++monom)
      {
        double prod = 1;
        for (unsigned int s = 0; s < dim; ++s)
          {
            if (p[s] < 0)
              Assert(std::floor(exponents[monom][s]) == exponents[monom][s],
                     ExcMessage("Exponentiation of a negative base number with "
                                "a real exponent can't be performed."));
            prod *= std::pow(p[s], exponents[monom][s]);
          }
        sum += coefficients[monom] * prod;
      }
    return sum;
  }



  template <int dim>
  void
  Polynomial<dim>::value_list(const std::vector<Point<dim>> &points,
                              std::vector<double>           &values,
                              const unsigned int             component) const
  {
    Assert(values.size() == points.size(),
           ExcDimensionMismatch(values.size(), points.size()));

    for (unsigned int i = 0; i < points.size(); ++i)
      values[i] = Polynomial<dim>::value(points[i], component);
  }



  template <int dim>
  Tensor<1, dim>
  Polynomial<dim>::gradient(const Point<dim>  &p,
                            const unsigned int component) const
  {
    AssertIndexRange(component, 1);

    Tensor<1, dim> r;

    for (unsigned int d = 0; d < dim; ++d)
      {
        double sum = 0;

        for (unsigned int monom = 0; monom < exponents.n_rows(); ++monom)
          {
            double prod = 1;
            for (unsigned int s = 0; s < dim; ++s)
              {
                if ((s == d) && (exponents[monom][s] == 0) && (p[s] == 0))
                  {
                    prod = 0;
                    break;
                  }
                else
                  {
                    if (p[s] < 0)
                      Assert(std::floor(exponents[monom][s]) ==
                               exponents[monom][s],
                             ExcMessage(
                               "Exponentiation of a negative base number with "
                               "a real exponent can't be performed."));
                    prod *=
                      (s == d ? exponents[monom][s] *
                                  std::pow(p[s], exponents[monom][s] - 1) :
                                std::pow(p[s], exponents[monom][s]));
                  }
              }
            sum += coefficients[monom] * prod;
          }
        r[d] = sum;
      }
    return r;
  }



  template <int dim>
  std::size_t
  Polynomial<dim>::memory_consumption() const
  {
    return sizeof(*this) + exponents.memory_consumption() - sizeof(exponents) +
           MemoryConsumption::memory_consumption(coefficients) -
           sizeof(coefficients);
  }

  template <int dim>
  RayleighKotheVortex<dim>::RayleighKotheVortex(const double T)
    : Function<dim>(dim)
    , T(T)
  {
    AssertThrow(dim > 1, ExcNotImplemented());
  }


  template <int dim>
  void
  RayleighKotheVortex<dim>::vector_value(const Point<dim> &point,
                                         Vector<double>   &values) const
  {
    const double pi_x = numbers::PI * point[0];
    const double pi_y = numbers::PI * point[1];
    const double pi_t = numbers::PI / T * this->get_time();

    values[0] = -2 * std::cos(pi_t) *
                Utilities::fixed_power<2>(std::sin(pi_x)) * std::sin(pi_y) *
                std::cos(pi_y);
    values[1] = +2 * std::cos(pi_t) *
                Utilities::fixed_power<2>(std::sin(pi_y)) * std::sin(pi_x) *
                std::cos(pi_x);

    if (dim == 3)
      values[2] = 0;
  }


  // explicit instantiations
  template class SquareFunction<1>;
  template class SquareFunction<2>;
  template class SquareFunction<3>;
  template class Q1WedgeFunction<1>;
  template class Q1WedgeFunction<2>;
  template class Q1WedgeFunction<3>;
  template class PillowFunction<1>;
  template class PillowFunction<2>;
  template class PillowFunction<3>;
  template class CosineFunction<1>;
  template class CosineFunction<2>;
  template class CosineFunction<3>;
  template class CosineGradFunction<1>;
  template class CosineGradFunction<2>;
  template class CosineGradFunction<3>;
  template class ExpFunction<1>;
  template class ExpFunction<2>;
  template class ExpFunction<3>;
  template class JumpFunction<1>;
  template class JumpFunction<2>;
  template class JumpFunction<3>;
  template class FourierCosineFunction<1>;
  template class FourierCosineFunction<2>;
  template class FourierCosineFunction<3>;
  template class FourierSineFunction<1>;
  template class FourierSineFunction<2>;
  template class FourierSineFunction<3>;
  template class FourierCosineSum<1>;
  template class FourierCosineSum<2>;
  template class FourierCosineSum<3>;
  template class FourierSineSum<1>;
  template class FourierSineSum<2>;
  template class FourierSineSum<3>;
  template class SlitSingularityFunction<2>;
  template class SlitSingularityFunction<3>;
  template class Monomial<1>;
  template class Monomial<2>;
  template class Monomial<3>;
  template class Monomial<1, float>;
  template class Monomial<2, float>;
  template class Monomial<3, float>;
  template class Bessel1<1>;
  template class Bessel1<2>;
  template class Bessel1<3>;
  template class InterpolatedTensorProductGridData<1>;
  template class InterpolatedTensorProductGridData<2>;
  template class InterpolatedTensorProductGridData<3>;
  template class InterpolatedUniformGridData<1>;
  template class InterpolatedUniformGridData<2>;
  template class InterpolatedUniformGridData<3>;
  template class Polynomial<1>;
  template class Polynomial<2>;
  template class Polynomial<3>;
  template class RayleighKotheVortex<1>;
  template class RayleighKotheVortex<2>;
  template class RayleighKotheVortex<3>;
} // namespace Functions

DEAL_II_NAMESPACE_CLOSE
