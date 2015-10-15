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
   * All allocation requests are passed to the Primary allocator. Only if this
   * cannot fulfill the request, it is passed to the Fallback allocator
   * \tparam Primary The allocator that gets all requests by default
   * \tparam Fallback The allocator that get the requests, if the Primary failed.
   *
   * \ingroup group_allocators group_shared
   */
  template <class Primary, class Fallback>
  class fallback_allocator : public Primary, public Fallback {
    using primary = Primary;
    using fallback = Fallback;

    static_assert(!traits::both_same_base<Primary, Fallback>::value,
                  "Primary- and Fallback-Allocator cannot be both of the same base!");

  public:
    static const bool supports_truncated_deallocation = Primary::supports_truncated_deallocation ||
      Fallback::supports_truncated_deallocation;

    /**
     * Allocates the requested number of bytes.
     * \param n The number of bytes. Depending on the alignment of the allocator,
     *          the block might contain a bigger size
     */
    block allocate(size_t n) noexcept
    {
      if (n == 0) {
        return {};
      }
      block result{Primary::allocate(n)};
      if (!result)
        result = Fallback::allocate(n);

      return result;
    }

    /**
     * Frees the memory of the provided block and resets it.
     * \param b The block describing the memory to be freed.
     */
    void deallocate(block &b) noexcept
    {
      if (!b) {
        return;
      }

      if (Primary::owns(b))
        Primary::deallocate(b);
      else
        Fallback::deallocate(b);
    }

    /**
     * Reallocates the given block to the given size. If the Primary cannot handle
     * the request, then a memory move is done.
     * \param b The block to be reallocated
     * \param n The new size (Zero means deallocation.)
     * \return True if the operation was successful
     */
    bool reallocate(block &b, size_t n) noexcept
    {
      if (Primary::owns(b)) {
        if (internal::reallocator<Primary>::isHandledDefault(static_cast<Primary &>(*this), b, n)) {
          return true;
        }
      }
      else {
        if (internal::reallocator<Fallback>::isHandledDefault(static_cast<Fallback &>(*this), b,
                                                              n)) {
          return true;
        }
      }

      if (Primary::owns(b)) {
        if (Primary::reallocate(b, n)) {
          return true;
        }
        return internal::reallocateWithCopy(static_cast<Primary &>(*this),
                                            static_cast<Fallback &>(*this), b, n);
      }

      return Fallback::reallocate(b, n);
    }

    /**
     * Expands the given block by the given amount of bytes.
     * This method is only available if at least one of the allocators implements
     * it
     * \param b The block that should be expanded
     * \param delta The number of bytes that should be appended
     * \return True, if the operation could be performed successful.
     */
    template <typename U = Primary, typename V = Fallback>
    typename std::enable_if<traits::has_expand<U>::value || traits::has_expand<V>::value, bool>::type
    expand(block &b, size_t delta) noexcept
    {
      if (Primary::owns(b)) {
        if (traits::has_expand<U>::value) {
          return traits::Expander<U>::doIt(static_cast<U&>(*this), b, delta);
        }
        return false;
      }
      if (traits::has_expand<V>::value) {
        return traits::Expander<V>::doIt(static_cast<V&>(*this), b, delta);
      }
      return false;
    }

    /**
     * Checks for the ownership of the given block
     * This method is only available, if both allocators implements it.
     * \param b The block which ownership shall be checked.
     * \return True if the block comes from one of the allocators.
     */
    template <typename U = Primary, typename V = Fallback>
    typename std::enable_if<traits::has_owns<U>::value && traits::has_owns<V>::value, bool>::type
    owns(const block &b) const noexcept
    {
      return Primary::owns(b) || Fallback::owns(b);
    }

    template <typename U = Primary, typename V = Fallback>
    typename std::enable_if<traits::has_deallocateAll<U>::value &&
                                                traits::has_deallocateAll<V>::value, void>::type
    deallocateAll() noexcept
    {
      Primary::deallocateAll();
      Fallback::deallocateAll();
    }
  };
}
