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
#include <boost/config/suffix.hpp>

namespace alb {

  /**
 * This class implements a facade against the system ::malloc()
 *
 * \ingroup group_allocators group_shared
 */
class mallocator {

public:
  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation = false);
  /**
   * Allocates the specified number of bytes.
   * If the system cannot allocate the specified amount of memory then
   * a null Block is returned.
   * \param n The number of bytes.
   * \return Block with memory information
   */
  Block allocate(size_t n) {
    if (n == 0) {
      return Block();
    }
    void *p = ::malloc(n);
    if (p != nullptr) {
      return Block(p, n);
    }
    return Block();
  }

  /**
   * Reallocate the specified block to the specified size.
   * \param b The block to be reallocated
   * \param n The new size
   * \return True, if the operation was successful.
   */
  bool reallocate(Block &b, size_t n) {
    if (helper::Reallocator<mallocator>::isHandledDefault(*this, b, n)) {
      return true;
    }

    Block reallocatedBlock(::realloc(b.ptr, n), n);

    if (reallocatedBlock.ptr != nullptr) {
      b = reallocatedBlock;
      return true;
    }
    return false;
  }

  /**
   * Frees the given block and resets it.
   * \param b Block to be freed.
   */
  void deallocate(Block &b) {
    if (b) {
      ::free(b.ptr);
      b.reset();
    }
  }
};

}
