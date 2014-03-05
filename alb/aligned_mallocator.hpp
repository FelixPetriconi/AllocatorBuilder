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
 * This class implements a proxy to the system ::malloc() with the ALB interface. 
 * According the template * parameter DefaultAlignment the allocated values are 
 * aligned to the specific boundary in bytes. Normally this should be a multiple 
 * of at least 4 bytes.
 * \tparam DefaultAlignment Specified the alignment in bytes of all allocation
 *         and reallocations.
 *
 * \ingroup group_allocators group_shared
 */
template <size_t DefaultAlignment = 16> 
class aligned_mallocator {
  BOOST_STATIC_CONSTANT(unsigned, alignment = DefaultAlignment);

#ifdef BOOST_MSVC
  bool alignedReallocate(block &b, size_t n) {
    block reallocatedBlock(_aligned_realloc(b.ptr, n, DefaultAlignment), n);

    if (reallocatedBlock.ptr != nullptr) {
      b = reallocatedBlock;
      return true;
    }
    return false;
  }
#else
  // On posix there is no _aligned_realloc so we try a normal realloc
  // if the result is still aligned we are fine
  // otherwise we have to do it by hand
  bool alignedReallocate(block &b, size_t n) {
    block reallocatedBlock(::realloc(b.ptr, n));
    if (reallocatedBlock.ptr != nullptr) {
      if (static_cast<size_t>(b.ptr) % DefaultAlignment != 0) {
        auto newAlignedBlock = allocate(n);
        if (!newAlignedBlock) {
          return false;
        }
        internal::blockCopy(b, newAlignedBlock);
      } else {
        b = reallocatedBlock;
      }

      return true;
    }
    return false;
  }
#endif

public:
  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation = false);
  /**
   * Allocates rounded up to the defined alignment the number of bytes.
   * If the system cannot allocate the specified amount of memory then
   * a null Block is returned.
   */
  block allocate(size_t n) {
#ifdef BOOST_MSVC
    return block(_aligned_malloc(n, DefaultAlignment), n);
#else
    return block((void*)memalign(DefaultAlignment, n), n);
#endif
  }

  /**
   * The given block is reallocated to the given size. The new size is aligned
   * to
   * specified alignment. It depends to the OS, if the provided memory block is
   * expanded or moved. It is guaranteed that the values of min(b.length, n)
   * bytes are preserved.
   * \param b The block that should be reallocated
   * \param n The new size of the block
   * \return True, if the operation was successful
   */
  bool reallocate(block &b, size_t n) {
    if (internal::reallocator<aligned_mallocator>::isHandledDefault(*this, b, n)) {
      return true;
    }

    return alignedReallocate(b, n);
  }

  /**
   * Frees the given block back to the system. The block gets nulled.
   * \param b The block, describing what memory shall be freed.
   */
  void deallocate(block &b) {
    if (b) {
#ifdef BOOST_MSVC
      _aligned_free(b.ptr);
#else
      ::free(b.ptr);
#endif
      b.reset();
    }
  }
};
}
