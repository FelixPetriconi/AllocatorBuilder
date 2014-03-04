///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#pragma once

#include "allocator_base.hpp"

#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <boost/config/suffix.hpp>
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
#include <boost/noncopyable.hpp>
#endif

namespace ALB {
/**
 * Allocator that provides memory from the stack.
 * By design it is not thread safe!
 * \tparam MaxSize The maximum number of bytes that can be allocated by this
 *         allocator
 * \tparam Alignment Each memory allocation request by  allocate,
 *         reallocate and expand is aligned by this value
 *
 * \ingroup group_allocators
 */
template <size_t MaxSize, size_t Alignment = 4> 
class StackAllocator 
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
  : boost::noncopyable
#endif
{
  char _data[MaxSize];
  char *_p;

  bool isLastUsedBlock(const Block &block) const {
    return (static_cast<char *>(block.ptr) + block.length == _p);
  }

public:
  typedef StackAllocator allocator;

  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation = true);

  BOOST_STATIC_CONSTANT(size_t, max_size = MaxSize);

  BOOST_STATIC_CONSTANT(size_t, alignment = Alignment);

  StackAllocator() : _p(_data) {}

  Block allocate(size_t n) {
    Block result;

    if (n == 0) {
      return result;
    }

    const auto alignedLength = Helper::roundToAlignment(Alignment, n);
    if (alignedLength + _p > _data + MaxSize) { // not enough memory left
      return result;
    }

    result.ptr = _p;
    result.length = alignedLength;

    _p += alignedLength;
    return result;
  }

  void deallocate(Block &b) {
    if (!b) {
      return;
    }
    if (!owns(b)) {
      BOOST_ASSERT(false);
      return;
    }

    // If it was the most recent allocated MemoryBlock, then we can re-use the
    // memory. Otherwise this freed MemoryBlock is not available for further
    // allocations. Since all happens on the stack this is not a leak!
    if (isLastUsedBlock(b)) {
      _p = static_cast<char *>(b.ptr);
    }
    b.reset();
  }

  bool reallocate(Block &b, size_t n) {
    if (b.length == n) {
      return true;
    }

    if (n == 0) {
      deallocate(b);
      return true;
    }

    if (!b) {
      b = allocate(n);
      return true;
    }

    const auto alignedLength = Helper::roundToAlignment(Alignment, n);

    if (isLastUsedBlock(b)) {
      if (static_cast<char *>(b.ptr) + alignedLength <= _data + MaxSize) {
        b.length = alignedLength;
        _p = static_cast<char *>(b.ptr) + alignedLength;
        return true;
      } else { // out of memory
        return false;
      }
    } else {
      if (b.length > n) {
        b.length = Helper::roundToAlignment(Alignment, n);
        return true;
      }
    }

    auto newBlock = allocate(alignedLength);
    // we cannot deallocate the old block, because it is in between used ones,
    //  so we have to "leak" here.
    if (newBlock) {
      Helper::blockCopy(b, newBlock);
      b = newBlock;
      return true;
    }
    return false;
  }

  /**
   * Expands the given block insito by the amount of bytes
   * \param b The block that should be expanded
   * \param delta The amount of bytes that should be appended
   * \return true, if the operation was successful or false if not enough
   *         memory is left
   */
  bool expand(Block &b, size_t delta) {
    if (delta == 0) {
      return true;
    }
    if (!b) {
      b = allocate(delta);
      return b.length != 0;
    }
    if (!isLastUsedBlock(b)) {
      return false;
    }
    auto alignedBytes = Helper::roundToAlignment(Alignment, delta);
    if (_p + alignedBytes > _data + MaxSize) {
      return false;
    }
    _p += alignedBytes;
    b.length += alignedBytes;
    return true;
  }

  /**
   * Returns true, if the provided block was allocated previously with this
   * allocator
   * \param b The block to be checked.
   */
  bool owns(const Block &b) const {
    return b && (b.ptr >= _data && b.ptr < _data + MaxSize);
  }

  /**
   * Sets all possibly provided memory to free.
   * Be warned that all usage of previously allocated blocks results in
   * unpredictable results!
   */
  void deallocateAll() { _p = _data; }

private:
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
# define DELETED
#else
# define DELETED =delete
#endif
  
  // disable move ctor and move assignment operators
  StackAllocator(StackAllocator &&) DELETED;
  StackAllocator &operator=(StackAllocator &&) DELETED;
  // disable heap allocation
  void *operator new(size_t)  DELETED;
  void *operator new[](size_t)  DELETED;
  void operator delete(void *)  DELETED;
  void operator delete[](void *)  DELETED;
  // disable address taking
  StackAllocator *operator&() DELETED;

#undef DELETED
};

template <size_t MaxSize, size_t Alignment> 
const size_t StackAllocator<MaxSize, Alignment>::max_size;
template <size_t MaxSize, size_t Alignment> 
const size_t StackAllocator<MaxSize, Alignment>::alignment;

}

