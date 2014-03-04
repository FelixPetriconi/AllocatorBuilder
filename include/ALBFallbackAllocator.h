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
#include <boost/type_traits/ice.hpp>

namespace ALB {
/**
 * All allocation requests are passed to the Primary allocator. Only if this
 * cannot fulfill the request, it is passed to the Fallback allocator
 * \tparam Primary The allocator that gets all requests by default
 * \tparam Fallback The allocator that get the requests, if the Primary failed.
 *
 * \ingroup group_allocators group_shared
 */
template <class Primary, class Fallback>
class FallbackAllocator : public Primary, public Fallback {
  typedef Primary primary_allocator;
  typedef Fallback fallback_allocator;

  static_assert(
      !Traits::both_same_base<Primary, Fallback>::value,
      "Primary- and Fallback-Allocator cannot be both of the same base!");

public:
  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation =
    (::boost::type_traits::ice_or<Primary::supports_truncated_deallocation, 
      Fallback::supports_truncated_deallocation>::value));
  /**
   * Allocates the requested number of bytes.
   * \param n The number of bytes. Depending on the alignment of the allocator,
   *          the block might contain a bigger size
   */
  Block allocate(size_t n) {
    if (n == 0) {
      return Block();
    }
    Block result(Primary::allocate(n));
    if (!result)
      result = Fallback::allocate(n);

    return result;
  }

  /**
   * Frees the memory of the provided block and resets it.
   * \param b The block describing the memory to be freed.
   */
  void deallocate(Block &b) {
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
  bool reallocate(Block &b, size_t n) {
    if (Primary::owns(b)) {
      if (Helper::Reallocator<Primary>::isHandledDefault(
              static_cast<Primary &>(*this), b, n)) {
        return true;
      }
    } else {
      if (Helper::Reallocator<Fallback>::isHandledDefault(
              static_cast<Fallback &>(*this), b, n)) {
        return true;
      }
    }

    if (Primary::owns(b)) {
      if (Primary::reallocate(b, n)) {
        return true;
      }
      return Helper::reallocateWithCopy(static_cast<Primary &>(*this),
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
  typename Traits::enable_result_to < bool,
      Traits::has_expand<Primary>::value ||
          Traits::has_expand<Fallback>::value >
              ::type expand(Block &b, size_t delta) {
    if (Primary::owns(b)) {
      if (Traits::has_expand<Primary>::value) {
        return Traits::Expander<Primary>::doIt(static_cast<Primary &>(*this), b,
                                               delta);
      } else {
        return false;
      }
    }
    if (Traits::has_expand<Fallback>::value) {
      return Traits::Expander<Fallback>::doIt(static_cast<Fallback &>(*this), b,
                                              delta);
    }
    return false;
  }

  /**
   * Checks for the ownership of the given block
   * This method is only available, if both allocators implements it.
   * \param b The block which ownership shall be checked.
   * \return True if the block comes from one of the allocators.
   */
  typename Traits::enable_result_to<bool,
                                    Traits::has_owns<Primary>::value &&
                                        Traits::has_owns<Fallback>::value>::type
  owns(const Block &b) {
    return Primary::owns(b) || Fallback::owns(b);
  }

  typename Traits::enable_result_to<
      void, Traits::has_deallocateAll<Primary>::value &&
                Traits::has_deallocateAll<Fallback>::value>::type
  deallocateAll() {
    Primary::deallocateAll();
    Fallback::deallocateAll();
  }
};
}
