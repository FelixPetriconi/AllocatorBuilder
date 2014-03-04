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

#include "ALBAllocatorBase.h"
#include <boost/assert.hpp>
#include <boost/config/suffix.hpp>
#include <boost/type_traits/ice.hpp>

namespace ALB {

/**
 * This allocator separates the allocation requested depending on a threshold
 * between the Small- and the LargeAllocator
 * \tparam Threshold The edge until all allocations go to the SmallAllocator
 * \tparam SmallAllocator This gets all allocations below the  Threshold
 * \tparam LargeAllocator This gets all allocations starting with the Threshold
 * 
 * \ingroup group_allocators group_shared
 */
template <size_t Threshold, class SmallAllocator, class LargeAllocator>
class Segregator : private SmallAllocator, private LargeAllocator {
  static_assert(!traits::both_same_base<SmallAllocator, LargeAllocator>::value,
                "Small- and Large-Allocator cannot be both of base!");

public:
  typedef SmallAllocator small_allocator;
  typedef LargeAllocator large_allocator;

  BOOST_STATIC_CONSTANT(size_t, threshold = Threshold);

  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation =
      (::boost::type_traits::ice_and<SmallAllocator::supports_truncated_deallocation, 
      LargeAllocator::supports_truncated_deallocation>::value));

  /**
   * Allocates the specified number of bytes. If the operation was not
   * successful
   * it returns an empty block.
   * \param n Number of requested bytes
   * \return Block with the memory information.
   */
  Block allocate(size_t n) {
    if (n < Threshold) {
      return SmallAllocator::allocate(n);
    }
    return LargeAllocator::allocate(n);
  }

  /**
   * Frees the given block and resets it.
   * \param b The block to be freed.
   */
  void deallocate(Block &b) {
    if (!b) {
      return;
    }

    if (!owns(b)) {
      BOOST_ASSERT_MSG(false,
                       "It is not wise to pass me a foreign allocated block!");
      return;
    }

    if (b.length < Threshold) {
      return SmallAllocator::deallocate(b);
    }
    return LargeAllocator::deallocate(b);
  }

  /**
   * Reallocates the given block to the given size. If the new size crosses the
   * Threshold, then a memory move will be performed.
   * \param b The block to be changed
   * \param n The new size
   * \return True, if the operation was successful
   * \ingroup group_allocators group_shared
   */
  bool reallocate(Block &b, size_t n) {
    if (!owns(b)) {
      BOOST_ASSERT_MSG(false,
                       "It is not wise to pass me a foreign allocated block!");
      return false;
    }
    if (Helper::Reallocator<Segregator>::isHandledDefault(*this, b, n)) {
      return true;
    }

    if (b.length < Threshold) {
      if (n < Threshold) {
        return SmallAllocator::reallocate(b, n);
      } else {
        return Helper::reallocateWithCopy(
            *this, static_cast<LargeAllocator &>(*this), b, n);
      }
    } else {
      if (n < Threshold) {
        return Helper::reallocateWithCopy(
            *this, static_cast<SmallAllocator &>(*this), b, n);
      } else {
        return SmallAllocator::reallocate(b, n);
      }
    }

    return false;
  }

  /**
   * The given block will be expanded insito
   * This method is only available if one the Allocators implements it.
   * \param b The block to be expanded
   * \param delta The number of bytes to be expanded
   * \return True, if the operation was successful
   */
  typename traits::enable_result_to < bool,
      traits::has_expand<SmallAllocator>::value ||
          traits::has_expand<LargeAllocator>::value >
              ::type expand(Block &b, size_t delta) {
    if (b.length < Threshold && b.length + delta >= Threshold) {
      return false;
    }
    if (b.length < Threshold) {
      if (traits::has_expand<SmallAllocator>::value) {
        return traits::Expander<SmallAllocator>::doIt(
            static_cast<SmallAllocator &>(*this), b, delta);
      } else {
        return false;
      }
    }
    if (traits::has_expand<LargeAllocator>::value) {
      return traits::Expander<LargeAllocator>::doIt(
          static_cast<LargeAllocator &>(*this), b, delta);
    } else {
      return false;
    }
  }

  /**
   * Checks the ownership of the given block.
   * This is only available if both Allocator implement it
   * \param b The block to checked
   * \return True if one of the allocator owns it.
   */
  typename traits::enable_result_to<
      bool, traits::has_owns<SmallAllocator>::value &&
                traits::has_owns<LargeAllocator>::value>::type
  owns(const Block &b) const {
    if (b.length < Threshold) {
      return SmallAllocator::owns(b);
    }
    return LargeAllocator::owns(b);
  }

  /**
   * Deallocates all memory.
   * This is available if one of the allocators implement it.
   */
  typename traits::enable_result_to < bool,
      traits::has_deallocateAll<SmallAllocator>::value ||
          traits::has_deallocateAll<LargeAllocator>::value >
              ::type deallocateAll() {
    traits::AllDeallocator<SmallAllocator>::doIt(
        static_cast<SmallAllocator &>(*this));
    traits::AllDeallocator<LargeAllocator>::doIt(
        static_cast<LargeAllocator &>(*this));
  }
};
}
