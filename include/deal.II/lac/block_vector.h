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

#ifndef dealii_block_vector_h
#define dealii_block_vector_h


#include <deal.II/base/config.h>

#include <deal.II/base/exceptions.h>

#include <deal.II/lac/block_indices.h>
#include <deal.II/lac/block_vector_base.h>
#include <deal.II/lac/vector_operation.h>
#include <deal.II/lac/vector_type_traits.h>

#include <cstdio>
#include <vector>

DEAL_II_NAMESPACE_OPEN


// Forward declaration
#ifndef DOXYGEN
#  ifdef DEAL_II_WITH_TRILINOS
namespace TrilinosWrappers
{
  namespace MPI
  {
    class BlockVector;
  }
} // namespace TrilinosWrappers
#  endif
#endif


/**
 * @addtogroup Vectors
 * @{
 */


/**
 * An implementation of block vectors based on deal.II vectors. While the base
 * class provides for most of the interface, this class handles the actual
 * allocation of vectors and provides functions that are specific to the
 * underlying vector type.
 *
 * @note Instantiations for this template are provided for <tt>@<float@> and
 * @<double@></tt>; others can be generated in application programs (see the
 * section on
 * @ref Instantiations
 * in the manual).
 *
 * @see
 * @ref GlossBlockLA "Block (linear algebra)"
 */
template <typename Number>
class BlockVector : public BlockVectorBase<Vector<Number>>
{
public:
  /**
   * Typedef the base class for simpler access to its own alias.
   */
  using BaseClass = BlockVectorBase<Vector<Number>>;

  /**
   * Typedef the type of the underlying vector.
   */
  using BlockType = typename BaseClass::BlockType;

  /**
   * Import the alias from the base class.
   */
  using value_type      = typename BaseClass::value_type;
  using real_type       = typename BaseClass::real_type;
  using pointer         = typename BaseClass::pointer;
  using const_pointer   = typename BaseClass::const_pointer;
  using reference       = typename BaseClass::reference;
  using const_reference = typename BaseClass::const_reference;
  using size_type       = typename BaseClass::size_type;
  using iterator        = typename BaseClass::iterator;
  using const_iterator  = typename BaseClass::const_iterator;

  /**
   * Constructor. There are three ways to use this constructor. First, without
   * any arguments, it generates an object with no blocks. Given one argument,
   * it initializes <tt>n_blocks</tt> blocks, but these blocks have size zero.
   * The third variant finally initializes all blocks to the same size
   * <tt>block_size</tt>.
   *
   * Confer the other constructor further down if you intend to use blocks of
   * different sizes.
   */
  explicit BlockVector(const unsigned int n_blocks   = 0,
                       const size_type    block_size = 0);

  /**
   * Copy Constructor. Dimension set to that of @p v, all components are
   * copied from @p v.
   */
  BlockVector(const BlockVector<Number> &V);


  /**
   * Move constructor. Creates a new vector by stealing the internal data of
   * the given argument vector.
   */
  BlockVector(BlockVector<Number> && /*v*/) noexcept = default;

  /**
   * Copy constructor taking a BlockVector of another data type. This will
   * fail if there is no conversion path from <tt>OtherNumber</tt> to
   * <tt>Number</tt>. Note that you may lose accuracy when copying to a
   * BlockVector with data elements with less accuracy.
   */
  template <typename OtherNumber>
  explicit BlockVector(const BlockVector<OtherNumber> &v);

#ifdef DEAL_II_WITH_TRILINOS
  /**
   * A copy constructor taking a (parallel) Trilinos block vector and copying
   * it into the deal.II own format.
   */
  BlockVector(const TrilinosWrappers::MPI::BlockVector &v);

#endif
  /**
   * Constructor. Set the number of blocks to <tt>block_sizes.size()</tt> and
   * initialize each block with <tt>block_sizes[i]</tt> zero elements.
   */
  BlockVector(const std::vector<size_type> &block_sizes);

  /**
   * Constructor. Initialize vector to the structure found in the BlockIndices
   * argument.
   */
  BlockVector(const BlockIndices &block_indices);

  /**
   * Constructor. Set the number of blocks to <tt>block_sizes.size()</tt>.
   * Initialize the vector with the elements pointed to by the range of
   * iterators given as second and third argument. Apart from the first
   * argument, this constructor is in complete analogy to the respective
   * constructor of the <tt>std::vector</tt> class, but the first argument is
   * needed in order to know how to subdivide the block vector into different
   * blocks.
   */
  template <typename InputIterator>
  BlockVector(const std::vector<size_type> &block_sizes,
              const InputIterator           first,
              const InputIterator           end);

  /**
   * Destructor. Clears memory
   */
  ~BlockVector() override = default;

  /**
   * Call the compress() function on all the subblocks.
   *
   * This functionality only needs to be called if using MPI based vectors and
   * exists in other objects for compatibility.
   *
   * See
   * @ref GlossCompress "Compressing distributed objects"
   * for more information.
   */
  void
  compress(VectorOperation::values operation = VectorOperation::unknown);

  /**
   * Returns `false` as this is a serial block vector.
   *
   * This functionality only needs to be called if using MPI based vectors and
   * exists in other objects for compatibility.
   */
  bool
  has_ghost_elements() const;

  /**
   * Copy operator: fill all components of the vector with the given scalar
   * value.
   */
  BlockVector &
  operator=(const value_type s);

  /**
   * Copy operator for arguments of the same type. Resize the present vector
   * if necessary to the correct number of blocks, then copy the individual
   * blocks from `v` using the copy-assignment operator of the class that
   * represents the individual blocks.
   */
  BlockVector<Number> &
  operator=(const BlockVector<Number> &v);

  /**
   * Move the given vector. This operator replaces the present vector with
   * the contents of the given argument vector.
   */
  BlockVector<Number> &
  operator=(BlockVector<Number> && /*v*/) = default; // NOLINT

  /**
   * Copy operator for template arguments of different types. Resize the
   * present vector if necessary.
   */
  template <class Number2>
  BlockVector<Number> &
  operator=(const BlockVector<Number2> &V);

  /**
   * Copy a regular vector into a block vector.
   */
  BlockVector<Number> &
  operator=(const Vector<Number> &V);

#ifdef DEAL_II_WITH_TRILINOS
  /**
   * A copy constructor from a Trilinos block vector to a deal.II block
   * vector.
   */
  BlockVector<Number> &
  operator=(const TrilinosWrappers::MPI::BlockVector &V);
#endif

  /**
   * Reinitialize the BlockVector to contain <tt>n_blocks</tt> blocks of size
   * <tt>block_size</tt> each.
   *
   * If the second argument is left at its default value, then the block
   * vector allocates the specified number of blocks but leaves them at zero
   * size. You then need to later reinitialize the individual blocks, and call
   * collect_sizes() to update the block system's knowledge of its individual
   * block's sizes.
   *
   * If <tt>omit_zeroing_entries==false</tt>, the vector is filled with zeros.
   */
  void
  reinit(const unsigned int n_blocks,
         const size_type    block_size           = 0,
         const bool         omit_zeroing_entries = false);

  /**
   * Reinitialize the BlockVector such that it contains
   * <tt>block_sizes.size()</tt> blocks. Each block is reinitialized to
   * dimension <tt>block_sizes[i]</tt>.
   *
   * If the number of blocks is the same as before this function was called,
   * all vectors remain the same and reinit() is called for each vector.
   *
   * If <tt>omit_zeroing_entries==false</tt>, the vector is filled with zeros.
   *
   * Note that you must call this (or the other reinit() functions) function,
   * rather than calling the reinit() functions of an individual block, to
   * allow the block vector to update its caches of vector sizes. If you call
   * reinit() on one of the blocks, then subsequent actions on this object may
   * yield unpredictable results since they may be routed to the wrong block.
   */
  void
  reinit(const std::vector<size_type> &block_sizes,
         const bool                    omit_zeroing_entries = false);

  /**
   * Reinitialize the BlockVector to reflect the structure found in
   * BlockIndices.
   *
   * If the number of blocks is the same as before this function was called,
   * all vectors remain the same and reinit() is called for each vector.
   *
   * If <tt>omit_zeroing_entries==false</tt>, the vector is filled with zeros.
   */
  void
  reinit(const BlockIndices &block_indices,
         const bool          omit_zeroing_entries = false);

  /**
   * Change the dimension to that of the vector <tt>V</tt>. The same applies
   * as for the other reinit() function.
   *
   * The elements of <tt>V</tt> are not copied, i.e.  this function is the
   * same as calling <tt>reinit (V.size(), omit_zeroing_entries)</tt>.
   *
   * Note that you must call this (or the other reinit() functions) function,
   * rather than calling the reinit() functions of an individual block, to
   * allow the block vector to update its caches of vector sizes. If you call
   * reinit() of one of the blocks, then subsequent actions of this object may
   * yield unpredictable results since they may be routed to the wrong block.
   */
  template <typename Number2>
  void
  reinit(const BlockVector<Number2> &V,
         const bool                  omit_zeroing_entries = false);

  /**
   * Multiply each element of this vector by the corresponding element of
   * <tt>v</tt>.
   */
  template <class BlockVector2>
  void
  scale(const BlockVector2 &v);

  /**
   * Swap the contents of this vector and the other vector <tt>v</tt>. One
   * could do this operation with a temporary variable and copying over the
   * data elements, but this function is significantly more efficient since it
   * only swaps the pointers to the data of the two vectors and therefore does
   * not need to allocate temporary storage and move data around.
   *
   * This function is analogous to the swap() function of all C++ standard
   * containers. Also, there is a global function swap(u,v) that simply calls
   * <tt>u.swap(v)</tt>, again in analogy to standard functions.
   */
  void
  swap(BlockVector<Number> &v) noexcept;

  /**
   * Print to a stream.
   */
  void
  print(std::ostream      &out,
        const unsigned int precision  = 3,
        const bool         scientific = true,
        const bool         across     = true) const;

  /**
   * Write the vector en bloc to a stream. This is done in a binary mode, so
   * the output is neither readable by humans nor (probably) by other
   * computers using a different operating system or number format.
   */
  void
  block_write(std::ostream &out) const;

  /**
   * Read a vector en block from a file. This is done using the inverse
   * operations to the above function, so it is reasonably fast because the
   * bitstream is not interpreted.
   *
   * The vector is resized if necessary.
   *
   * A primitive form of error checking is performed which will recognize the
   * bluntest attempts to interpret some data as a vector stored bitwise to a
   * file, but not more.
   */
  void
  block_read(std::istream &in);

  /**
   * @addtogroup Exceptions
   * @{
   */

  /**
   * Exception
   */
  DeclException0(ExcIteratorRangeDoesNotMatchVectorSize);
  /** @} */
};

/** @} */

#ifndef DOXYGEN
/*----------------------- Inline functions ----------------------------------*/



template <typename Number>
template <typename InputIterator>
BlockVector<Number>::BlockVector(const std::vector<size_type> &block_sizes,
                                 const InputIterator           first,
                                 const InputIterator           end)
{
  // first set sizes of blocks, but
  // don't initialize them as we will
  // copy elements soon
  reinit(block_sizes, true);
  InputIterator start = first;
  for (size_type b = 0; b < block_sizes.size(); ++b)
    {
      InputIterator end = start;
      std::advance(end, static_cast<signed int>(block_sizes[b]));
      std::copy(start, end, this->block(b).begin());
      start = end;
    };
  Assert(start == end, ExcIteratorRangeDoesNotMatchVectorSize());
}



template <typename Number>
inline BlockVector<Number> &
BlockVector<Number>::operator=(const value_type s)
{
  AssertIsFinite(s);

  BaseClass::operator=(s);
  return *this;
}



template <typename Number>
inline BlockVector<Number> &
BlockVector<Number>::operator=(const BlockVector<Number> &v)
{
  reinit(v, true);
  BaseClass::operator=(v);
  return *this;
}



template <typename Number>
inline BlockVector<Number> &
BlockVector<Number>::operator=(const Vector<Number> &v)
{
  BaseClass::operator=(v);
  return *this;
}



template <typename Number>
template <typename Number2>
inline BlockVector<Number> &
BlockVector<Number>::operator=(const BlockVector<Number2> &v)
{
  reinit(v, true);
  BaseClass::operator=(v);
  return *this;
}

template <typename Number>
inline void
BlockVector<Number>::compress(VectorOperation::values operation)
{
  for (size_type i = 0; i < this->n_blocks(); ++i)
    this->components[i].compress(operation);
}



template <typename Number>
inline bool
BlockVector<Number>::has_ghost_elements() const
{
  return false;
}



template <typename Number>
template <class BlockVector2>
void
BlockVector<Number>::scale(const BlockVector2 &v)
{
  BaseClass::scale(v);
}

#endif // DOXYGEN


/**
 * Global function which overloads the default implementation of the C++
 * standard library which uses a temporary object. The function simply
 * exchanges the data of the two vectors.
 *
 * @relatesalso BlockVector
 */
template <typename Number>
inline void
swap(BlockVector<Number> &u, BlockVector<Number> &v) noexcept
{
  u.swap(v);
}


namespace internal
{
  namespace LinearOperatorImplementation
  {
    template <typename>
    class ReinitHelper;

    /**
     * A helper class used internally in linear_operator.h. Specialization for
     * BlockVector<number>.
     */
    template <typename number>
    class ReinitHelper<BlockVector<number>>
    {
    public:
      template <typename Matrix>
      static void
      reinit_range_vector(const Matrix        &matrix,
                          BlockVector<number> &v,
                          bool                 omit_zeroing_entries)
      {
        v.reinit(matrix.get_row_indices(), omit_zeroing_entries);
      }

      template <typename Matrix>
      static void
      reinit_domain_vector(const Matrix        &matrix,
                           BlockVector<number> &v,
                           bool                 omit_zeroing_entries)
      {
        v.reinit(matrix.get_column_indices(), omit_zeroing_entries);
      }
    };

  } // namespace LinearOperatorImplementation
} /* namespace internal */


/**
 * Declare dealii::BlockVector as serial vector.
 */
template <typename Number>
struct is_serial_vector<BlockVector<Number>> : std::true_type
{};

DEAL_II_NAMESPACE_CLOSE

#endif
