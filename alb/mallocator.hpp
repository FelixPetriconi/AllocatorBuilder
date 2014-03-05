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
#include "internal/reallocator.hpp"
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
  block allocate(size_t n) {
    if (n == 0) {
      return block();
    }
    void *p = ::malloc(n);
    if (p != nullptr) {
      return block(p, n);
    }
    return block();
  }

  /**
   * Reallocate the specified block to the specified size.
   * \param b The block to be reallocated
   * \param n The new size
   * \return True, if the operation was successful.
   */
  bool reallocate(block &b, size_t n) {
    if (internal::reallocator<mallocator>::isHandledDefault(*this, b, n)) {
      return true;
    }

    block reallocatedBlock(::realloc(b.ptr, n), n);

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
  void deallocate(block &b) {
    if (b) {
      ::free(b.ptr);
      b.reset();
    }
  }
};

}
