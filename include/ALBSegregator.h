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

namespace ALB
{
  template <size_t Threshold, class SmallAllocator, class LargeAllocator>
  class Segregator : private SmallAllocator, private LargeAllocator {

    static const size_t threshold = Threshold;
    typename typedef SmallAllocator small_allocator;
    typename typedef LargeAllocator large_allocator;

    static_assert( !Traits::both_same_base<SmallAllocator, LargeAllocator>::value, "Small- and Large-Allocator cannot be both of base!");

  public:
    Block allocate(size_t n) {
      if (n < Threshold) {
        return SmallAllocator::allocate(n);
      }
      return LargeAllocator::allocate(n);
    }

    void deallocate(Block& b) {
      if (!b) {
        return;
      }

      if (!owns(b)) {
        BOOST_ASSERT(false);
        return;
      }

      if (b.length < Threshold) {
        return SmallAllocator::deallocate(b);
      }
      return LargeAllocator::deallocate(b);
    }

    bool reallocate(Block& b, size_t n) {
      if (!owns(b)) {
        BOOST_ASSERT(false);
        return false;
      }
      if (Helper::Reallocator<Segregator>::isHandledDefault(*this, b, n)){
        return true;
      }

      if (b.length < Threshold) {
        if (n < Threshold) {
          return SmallAllocator::reallocate(b, n);
        }
        else {
          return Helper::reallocateWithCopy(*this, static_cast<LargeAllocator&>(*this), b, n);
        }
      }
      else {
        if (n < Threshold) {
          return Helper::reallocateWithCopy(*this, static_cast<SmallAllocator&>(*this), b, n);
        }
        else {
          return SmallAllocator::reallocate(b, n);          
        }
      }

      return false;
    }

    // Make the function invisible for the has_expand<Segregator> trait if the dependent type
    // SmallAllocator, LargeAllocator do not implement expand
    typename Traits::enable_result_to<bool,
      Traits::has_expand<SmallAllocator>::value || Traits::has_expand<LargeAllocator>::value
    >::type expand(Block& b, size_t delta) 
    {
      if (b.length < Threshold && b.length + delta >= Threshold) {
        return false;
      }
      if (b.length < Threshold) {
        if (Traits::has_expand<SmallAllocator>::value) {
          return Traits::Expander<SmallAllocator>::doIt(static_cast<SmallAllocator&>(*this), b, delta);
        }
        else {
          return false;
        }
      }
      if (Traits::has_expand<LargeAllocator>::value) {
        return Traits::Expander<LargeAllocator>::doIt(static_cast<LargeAllocator&>(*this), b, delta);
      }
      else {
        return false;
      }
    }

    typename Traits::enable_result_to<bool,
      Traits::has_owns<SmallAllocator>::value && Traits::has_owns<LargeAllocator>::value
    >::type owns(const Block& b) const {
      if (b.length < Threshold) {
        return SmallAllocator::owns(b);
      }
      return LargeAllocator::owns(b);
    }

    typename Traits::enable_result_to<bool, 
      Traits::has_deallocateAll<SmallAllocator>::value || Traits::has_deallocateAll<LargeAllocator>::value
    >::type deallocateAll() {
      Traits::AllDeallocator<SmallAllocator>::doIt(static_cast<SmallAllocator&>(*this));
      Traits::AllDeallocator<LargeAllocator>::doIt(static_cast<LargeAllocator&>(*this));
    }
  };
}
 
