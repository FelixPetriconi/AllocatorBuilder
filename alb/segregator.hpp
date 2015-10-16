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

namespace alb {

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
  class segregator : private SmallAllocator, private LargeAllocator {
    static_assert(!traits::both_same_base<SmallAllocator, LargeAllocator>::value,
                  "Small- and Large-Allocator cannot be both of base!");

  public:
    using small_allocator = SmallAllocator;
    using large_allocator = LargeAllocator;

    static constexpr size_t threshold = Threshold;

    static constexpr bool supports_truncated_deallocation = 
      SmallAllocator::supports_truncated_deallocation && 
      LargeAllocator::supports_truncated_deallocation;

    /**
     * Allocates the specified number of bytes. If the operation was not
     * successful
     * it returns an empty block.
     * \param n Number of requested bytes
     * \return Block with the memory information.
     */
    block allocate(size_t n) noexcept
    {
      if (n <= Threshold) {
        return SmallAllocator::allocate(n);
      }
      return LargeAllocator::allocate(n);
    }

    /**
     * Frees the given block and resets it.
     * \param b The block to be freed.
     */
    void deallocate(block &b) noexcept
    {
      if (!b) {
        return;
      }

      if (b.length <= Threshold) {
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
     *
     * \ingroup group_allocators group_shared
     */
    bool reallocate(block &b, size_t n) noexcept
    {
      if (internal::reallocator<segregator>::is_handled_default(*this, b, n)) {
        return true;
      }

      if (b.length <= Threshold) {
        if (n <= Threshold) {
          return SmallAllocator::reallocate(b, n);
        }
        return internal::reallocate_with_copy(*this, static_cast<LargeAllocator &>(*this), b, n);
      }

      if (n <= Threshold) {
        return internal::reallocate_with_copy(*this, static_cast<SmallAllocator &>(*this), b, n);
      }
      return LargeAllocator::reallocate(b, n);
    }

    /**
     * The given block will be expanded insito
     * This method is only available if one the Allocators implements it.
     * \param b The block to be expanded
     * \param delta The number of bytes to be expanded
     * \return True, if the operation was successful
     */
    template <typename U = SmallAllocator, typename V = LargeAllocator>
    typename std::enable_if<traits::has_expand<SmallAllocator>::value ||
                            traits::has_expand<LargeAllocator>::value, bool>::type
    expand(block &b, size_t delta) noexcept
    {
      if (b.length <= Threshold && b.length + delta > Threshold) {
        return false;
      }
      if (b.length <= Threshold) {
        if (traits::has_expand<U>::value) {
          return traits::Expander<U>::do_it(static_cast<U&>(*this), b,
                                                        delta);
        }
        return false;
      }
      if (traits::has_expand<V>::value) {
        return traits::Expander<V>::do_it(static_cast<V&>(*this), b,
                                                      delta);
      }
      return false;
    }

    /**
     * Checks the ownership of the given block.
     * This is only available if both Allocator implement it
     * \param b The block to checked
     * \return True if one of the allocator owns it.
     */
    template <typename U = SmallAllocator, typename V = LargeAllocator>
    typename std::enable_if<traits::has_expand<SmallAllocator>::value ||
      traits::has_expand<LargeAllocator>::value, bool>::type
      owns(const block &b) const noexcept
    {
      if (b.length <= Threshold) {
        return U::owns(b);
      }
      return V::owns(b);
    }

    /**
     * Deallocates all memory.
     * This is available if one of the allocators implement it.
     */
    template <typename U = SmallAllocator, typename V = LargeAllocator>
    typename std::enable_if<traits::has_expand<SmallAllocator>::value ||
      traits::has_expand<LargeAllocator>::value, void>::type
    deallocate_all() noexcept
    {
      traits::AllDeallocator<U>::do_it(static_cast<U&>(*this));
      traits::AllDeallocator<V>::do_it(static_cast<V&>(*this));
    }
  };
}
