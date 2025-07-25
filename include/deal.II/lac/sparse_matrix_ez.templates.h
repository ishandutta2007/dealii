// ------------------------------------------------------------------------
//
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2002 - 2025 by the deal.II authors
//
// This file is part of the deal.II library.
//
// Part of the source code is dual licensed under Apache-2.0 WITH
// LLVM-exception OR LGPL-2.1-or-later. Detailed license information
// governing the source code and code contributions can be found in
// LICENSE.md and CONTRIBUTING.md at the top level directory of deal.II.
//
// ------------------------------------------------------------------------

#ifndef dealii_sparse_matrix_ez_templates_h
#define dealii_sparse_matrix_ez_templates_h


#include <deal.II/base/config.h>

#include <deal.II/lac/sparse_matrix_ez.h>
#include <deal.II/lac/vector.h>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <iostream>

DEAL_II_NAMESPACE_OPEN

//---------------------------------------------------------------------------

template <typename number>
SparseMatrixEZ<number>::SparseMatrixEZ()
  : n_columns(0)
  , increment(1)
  , saved_default_row_length(0)
{}


template <typename number>
SparseMatrixEZ<number>::SparseMatrixEZ(const SparseMatrixEZ<number> &m)
  : EnableObserverPointer(m)
  , n_columns(0)
  , increment(m.increment)
  , saved_default_row_length(m.saved_default_row_length)
{
  Assert(m.m() == 0 && m.n() == 0,
         ExcMessage(
           "This constructor can only be called if the provided argument "
           "is an empty matrix. This constructor can not be used to "
           "copy-construct a non-empty matrix. Use the "
           "SparseMatrixEZ::copy_from() function for that purpose."));
}


template <typename number>
SparseMatrixEZ<number>::SparseMatrixEZ(const size_type    n_rows,
                                       const size_type    n_cols,
                                       const size_type    default_row_length,
                                       const unsigned int default_increment)
{
  reinit(n_rows, n_cols, default_row_length, default_increment);
}



template <typename number>
SparseMatrixEZ<number> &
SparseMatrixEZ<number>::operator=(const SparseMatrixEZ<number> &m)
{
  Assert(m.empty(),
         ExcMessage("This operator can only be called if the provided right "
                    "hand side is an empty matrix. This operator can not be "
                    "used to copy a non-empty matrix. Use the "
                    "SparseMatrixEZ::copy_from() function for that purpose."));
  return *this;
}


template <typename number>
SparseMatrixEZ<number> &
SparseMatrixEZ<number>::operator=(const double d)
{
  Assert(d == 0, ExcScalarAssignmentOnlyForZeroValue());

  typename std::vector<Entry>::iterator       e   = data.begin();
  const typename std::vector<Entry>::iterator end = data.end();

  while (e != end)
    {
      (e++)->value = 0.;
    }

  return *this;
}



template <typename number>
void
SparseMatrixEZ<number>::reinit(const size_type    n_rows,
                               const size_type    n_cols,
                               const size_type    default_row_length,
                               const unsigned int default_increment,
                               const size_type    reserve)
{
  clear();

  saved_default_row_length = default_row_length;
  increment                = default_increment;

  n_columns = n_cols;
  row_info.resize(n_rows);
  if (reserve != 0)
    data.reserve(reserve);
  data.resize(default_row_length * n_rows);

  for (size_type i = 0; i < n_rows; ++i)
    row_info[i].start = i * default_row_length;
}


template <typename number>
void
SparseMatrixEZ<number>::clear()
{
  n_columns = 0;
  row_info.resize(0);
  data.resize(0);
}


template <typename number>
bool
SparseMatrixEZ<number>::empty() const
{
  return ((n_columns == 0) && (row_info.empty()));
}


template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::vmult(Vector<somenumber>       &dst,
                              const Vector<somenumber> &src) const
{
  Assert(m() == dst.size(), ExcDimensionMismatch(m(), dst.size()));
  Assert(n() == src.size(), ExcDimensionMismatch(n(), src.size()));

  const size_type end_row = row_info.size();
  for (size_type row = 0; row < end_row; ++row)
    {
      const RowInfo                              &ri = row_info[row];
      typename std::vector<Entry>::const_iterator entry =
        data.begin() + ri.start;
      double s = 0.;
      for (unsigned short i = 0; i < ri.length; ++i, ++entry)
        {
          Assert(entry->column != Entry::invalid, ExcInternalError());
          s += entry->value * src(entry->column);
        }
      dst(row) = s;
    }
}


template <typename number>
number
SparseMatrixEZ<number>::l2_norm() const
{
  number         sum   = 0.;
  const_iterator start = begin();
  const_iterator final = end();

  while (start != final)
    {
      const double value = start->value();
      sum += value * value;
      ++start;
    }
  return std::sqrt(sum);
}



template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::Tvmult(Vector<somenumber>       &dst,
                               const Vector<somenumber> &src) const
{
  dst = 0.;
  Tvmult_add(dst, src);
}


template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::vmult_add(Vector<somenumber>       &dst,
                                  const Vector<somenumber> &src) const
{
  Assert(m() == dst.size(), ExcDimensionMismatch(m(), dst.size()));
  Assert(n() == src.size(), ExcDimensionMismatch(n(), src.size()));

  const size_type end_row = row_info.size();
  for (size_type row = 0; row < end_row; ++row)
    {
      const RowInfo                              &ri = row_info[row];
      typename std::vector<Entry>::const_iterator entry =
        data.begin() + ri.start;
      double s = 0.;
      for (unsigned short i = 0; i < ri.length; ++i, ++entry)
        {
          Assert(entry->column != Entry::invalid, ExcInternalError());
          s += entry->value * src(entry->column);
        }
      dst(row) += s;
    }
}

template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::Tvmult_add(Vector<somenumber>       &dst,
                                   const Vector<somenumber> &src) const
{
  Assert(n() == dst.size(), ExcDimensionMismatch(n(), dst.size()));
  Assert(m() == src.size(), ExcDimensionMismatch(m(), src.size()));

  const size_type end_row = row_info.size();
  for (size_type row = 0; row < end_row; ++row)
    {
      const RowInfo                              &ri = row_info[row];
      typename std::vector<Entry>::const_iterator entry =
        data.begin() + ri.start;
      for (unsigned short i = 0; i < ri.length; ++i, ++entry)
        {
          Assert(entry->column != Entry::invalid, ExcInternalError());
          dst(entry->column) += entry->value * src(row);
        }
    }
}


template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::precondition_Jacobi(Vector<somenumber>       &dst,
                                            const Vector<somenumber> &src,
                                            const number              om) const
{
  Assert(m() == n(), ExcNotQuadratic());
  Assert(dst.size() == n(), ExcDimensionMismatch(dst.size(), n()));
  Assert(src.size() == n(), ExcDimensionMismatch(src.size(), n()));

  somenumber                                         *dst_ptr = dst.begin();
  const somenumber                                   *src_ptr = src.begin();
  typename std::vector<RowInfo>::const_iterator       ri  = row_info.begin();
  const typename std::vector<RowInfo>::const_iterator end = row_info.end();

  for (; ri != end; ++dst_ptr, ++src_ptr, ++ri)
    {
      Assert(ri->diagonal != RowInfo::invalid_diagonal, ExcNoDiagonal());
      *dst_ptr = om * *src_ptr / data[ri->start + ri->diagonal].value;
    }
}



template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::precondition_SOR(Vector<somenumber>       &dst,
                                         const Vector<somenumber> &src,
                                         const number              om) const
{
  Assert(m() == n(), ExcNotQuadratic());
  Assert(dst.size() == n(), ExcDimensionMismatch(dst.size(), n()));
  Assert(src.size() == n(), ExcDimensionMismatch(src.size(), n()));

  somenumber                                         *dst_ptr = dst.begin();
  const somenumber                                   *src_ptr = src.begin();
  typename std::vector<RowInfo>::const_iterator       ri  = row_info.begin();
  const typename std::vector<RowInfo>::const_iterator end = row_info.end();

  for (; ri != end; ++dst_ptr, ++src_ptr, ++ri)
    {
      Assert(ri->diagonal != RowInfo::invalid_diagonal, ExcNoDiagonal());
      number          s       = *src_ptr;
      const size_type end_row = ri->start + ri->diagonal;
      for (size_type i = ri->start; i < end_row; ++i)
        s -= data[i].value * dst(data[i].column);

      *dst_ptr = om * s / data[ri->start + ri->diagonal].value;
    }
}



template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::precondition_TSOR(Vector<somenumber>       &dst,
                                          const Vector<somenumber> &src,
                                          const number              om) const
{
  Assert(m() == n(), ExcNotQuadratic());
  Assert(dst.size() == n(), ExcDimensionMismatch(dst.size(), n()));
  Assert(src.size() == n(), ExcDimensionMismatch(src.size(), n()));

  somenumber       *dst_ptr = dst.begin() + dst.size() - 1;
  const somenumber *src_ptr = src.begin() + src.size() - 1;
  typename std::vector<RowInfo>::const_reverse_iterator ri = row_info.rbegin();
  const typename std::vector<RowInfo>::const_reverse_iterator end =
    row_info.rend();

  for (; ri != end; --dst_ptr, --src_ptr, ++ri)
    {
      Assert(ri->diagonal != RowInfo::invalid_diagonal, ExcNoDiagonal());
      number          s       = *src_ptr;
      const size_type end_row = ri->start + ri->length;
      for (size_type i = ri->start + ri->diagonal + 1; i < end_row; ++i)
        s -= data[i].value * dst(data[i].column);

      *dst_ptr = om * s / data[ri->start + ri->diagonal].value;
    }
}



template <typename number>
template <typename somenumber>
void
SparseMatrixEZ<number>::precondition_SSOR(
  Vector<somenumber>       &dst,
  const Vector<somenumber> &src,
  const number              om,
  const std::vector<std::size_t> &) const
{
  Assert(m() == n(), ExcNotQuadratic());
  Assert(dst.size() == n(), ExcDimensionMismatch(dst.size(), n()));
  Assert(src.size() == n(), ExcDimensionMismatch(src.size(), n()));

  somenumber                                         *dst_ptr = dst.begin();
  const somenumber                                   *src_ptr = src.begin();
  typename std::vector<RowInfo>::const_iterator       ri;
  const typename std::vector<RowInfo>::const_iterator end = row_info.end();

  // Forward
  for (ri = row_info.begin(); ri != end; ++dst_ptr, ++src_ptr, ++ri)
    {
      Assert(ri->diagonal != RowInfo::invalid_diagonal, ExcNoDiagonal());
      number          s       = 0;
      const size_type end_row = ri->start + ri->diagonal;
      for (size_type i = ri->start; i < end_row; ++i)
        s += data[i].value * dst(data[i].column);

      *dst_ptr = *src_ptr - s * om;
      *dst_ptr /= data[ri->start + ri->diagonal].value;
    }
  // Diagonal
  dst_ptr = dst.begin();
  for (ri = row_info.begin(); ri != end; ++dst_ptr, ++ri)
    *dst_ptr *= om * (2. - om) * data[ri->start + ri->diagonal].value;

  // Backward
  typename std::vector<RowInfo>::const_reverse_iterator       rri;
  const typename std::vector<RowInfo>::const_reverse_iterator rend =
    row_info.rend();
  dst_ptr = dst.begin() + dst.size() - 1;
  for (rri = row_info.rbegin(); rri != rend; --dst_ptr, ++rri)
    {
      const size_type end_row = rri->start + rri->length;
      number          s       = 0;
      for (size_type i = rri->start + rri->diagonal + 1; i < end_row; ++i)
        s += data[i].value * dst(data[i].column);

      *dst_ptr -= s * om;
      *dst_ptr /= data[rri->start + rri->diagonal].value;
    }
}



template <typename number>
std::size_t
SparseMatrixEZ<number>::memory_consumption() const
{
  return sizeof(*this) + sizeof(size_type) * row_info.capacity() +
         sizeof(typename SparseMatrixEZ<number>::Entry) * data.capacity();
}



template <typename number>
typename SparseMatrixEZ<number>::size_type
SparseMatrixEZ<number>::get_row_length(const size_type row) const
{
  return row_info[row].length;
}



template <typename number>
typename SparseMatrixEZ<number>::size_type
SparseMatrixEZ<number>::n_nonzero_elements() const
{
  typename std::vector<RowInfo>::const_iterator       row    = row_info.begin();
  const typename std::vector<RowInfo>::const_iterator endrow = row_info.end();

  // Add up entries actually used
  size_type used = 0;
  for (; row != endrow; ++row)
    used += row->length;
  return used;
}



template <typename number>
void
SparseMatrixEZ<number>::compute_statistics(size_type              &used,
                                           size_type              &allocated,
                                           size_type              &reserved,
                                           std::vector<size_type> &used_by_line,
                                           const bool              full) const
{
  typename std::vector<RowInfo>::const_iterator       row    = row_info.begin();
  const typename std::vector<RowInfo>::const_iterator endrow = row_info.end();

  // Add up entries actually used
  used                 = 0;
  size_type max_length = 0;
  for (; row != endrow; ++row)
    {
      used += row->length;
      if (max_length < row->length)
        max_length = row->length;
    }

  // Number of entries allocated is
  // position of last entry used
  --row;
  allocated = row->start + row->length;
  reserved  = data.capacity();


  if (full)
    {
      used_by_line.resize(max_length + 1);

      for (row = row_info.begin(); row != endrow; ++row)
        {
          ++used_by_line[row->length];
        }
    }
}


template <typename number>
void
SparseMatrixEZ<number>::print(std::ostream &out) const
{
  AssertThrow(out.fail() == false, ExcIO());

  const_iterator       i = begin();
  const const_iterator e = end();
  while (i != e)
    {
      out << i->row() << '\t' << i->column() << '\t' << i->value() << std::endl;
      ++i;
    }
}


template <typename number>
void
SparseMatrixEZ<number>::print_formatted(std::ostream      &out,
                                        const unsigned int precision,
                                        const bool         scientific,
                                        const unsigned int width_,
                                        const char        *zero_string,
                                        const double       denominator,
                                        const char        *separator) const
{
  AssertThrow(out.fail() == false, ExcIO());
  Assert(m() != 0, ExcNotInitialized());
  Assert(n() != 0, ExcNotInitialized());

  unsigned int width = width_;

  std::ios::fmtflags old_flags     = out.flags();
  unsigned int       old_precision = out.precision(precision);

  if (scientific)
    {
      out.setf(std::ios::scientific, std::ios::floatfield);
      if (width == 0u)
        width = precision + 7;
    }
  else
    {
      out.setf(std::ios::fixed, std::ios::floatfield);
      if (width == 0u)
        width = precision + 2;
    }

  // TODO: Skip nonexistent entries
  for (size_type i = 0; i < m(); ++i)
    {
      for (size_type j = 0; j < n(); ++j)
        {
          const Entry *entry = locate(i, j);
          if (entry)
            out << std::setw(width) << entry->value * denominator << separator;
          else
            out << std::setw(width) << zero_string << separator;
        }
      out << std::endl;
    };

  // reset output format
  out.precision(old_precision);
  out.flags(old_flags);
}


template <typename number>
void
SparseMatrixEZ<number>::block_write(std::ostream &out) const
{
  AssertThrow(out.fail() == false, ExcIO());

  // first the simple objects,
  // bracketed in [...]
  out << '[' << row_info.size() << "][" << n_columns << "][" << data.size()
      << "][" << increment << "][";
  // then write out real data
  typename std::vector<RowInfo>::const_iterator r = row_info.begin();
  out.write(reinterpret_cast<const char *>(&*r),
            sizeof(RowInfo) * row_info.size());

  out << "][";

  typename std::vector<Entry>::const_iterator d = data.begin();
  out.write(reinterpret_cast<const char *>(&*d), sizeof(Entry) * data.size());

  out << ']';

  AssertThrow(out.fail() == false, ExcIO());
}


#define DEAL_II_CHECK_INPUT(in, a, c)                                        \
  {                                                                          \
    in >> c;                                                                 \
    AssertThrow(c == a, ExcMessage("Unexpected character in input stream")); \
  }

template <typename number>
void
SparseMatrixEZ<number>::block_read(std::istream &in)
{
  AssertThrow(in.fail() == false, ExcIO());

  char c;
  int  n;
  // first read in simple data
  DEAL_II_CHECK_INPUT(in, '[', c);
  in >> n;
  row_info.resize(n);

  DEAL_II_CHECK_INPUT(in, ']', c);
  DEAL_II_CHECK_INPUT(in, '[', c);
  in >> n_columns;

  DEAL_II_CHECK_INPUT(in, ']', c);
  DEAL_II_CHECK_INPUT(in, '[', c);
  in >> n;
  data.resize(n);

  DEAL_II_CHECK_INPUT(in, ']', c);
  DEAL_II_CHECK_INPUT(in, '[', c);
  in >> increment;

  DEAL_II_CHECK_INPUT(in, ']', c);
  DEAL_II_CHECK_INPUT(in, '[', c);

  // then read data
  in.read(reinterpret_cast<char *>(row_info.data()),
          sizeof(RowInfo) * row_info.size());

  DEAL_II_CHECK_INPUT(in, ']', c);
  DEAL_II_CHECK_INPUT(in, '[', c);

  in.read(reinterpret_cast<char *>(data.data()), sizeof(Entry) * data.size());

  DEAL_II_CHECK_INPUT(in, ']', c);
}

#undef DEAL_II_CHECK_INPUT



DEAL_II_NAMESPACE_CLOSE

#endif // dealii_sparse_matrix_ez_templates_h
