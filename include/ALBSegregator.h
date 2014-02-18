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

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<Segregator>::isHandledDefault(*this, b, n)){
        return true;
      }

      if (SmallAllocator::owns(b)) {
        if (SmallAllocator::reallocate(b, n)) {
          return true;
        }
        else { // try a cross reallocate
          return Helper::reallocateWithCopy(*this, static_cast<LargeAllocator&>(*this), b, n);
        }
      }
      else {
        if (LargeAllocator::reallocate(b, n)) {
          return true;
        }
      }

      return false;
    }

    // Make the function invisible for the has_expand<Segregator> trait if the dependent type
    // SmallAllocator, LargeAllocator do not implement expand
    typename Traits::expand_enabled<SmallAllocator, LargeAllocator>::type 
    expand(Block& b, size_t delta) {
      if (b.length + delta >= Threshold) {
        return false;
      }
      if (b.length < Threshold) {
        return SmallAllocator::expand(b, delta);
      }
      return LargeAllocator::expand(b, delta);
    }

    bool owns(const Block& b) const {
      if (b.length < Threshold) {
        return SmallAllocator::owns(b);
      }
      return LargeAllocator::owns(n);
    }

    void deallocate(Block& b) {
      if (!b) {
        return;
      }

      if (b.length < Threshold) {
        return SmallAllocator::deallocate(b);
      }
      return LargeAllocator::deallocate(b);
    }

    typename Traits::deallocateAll_enabled<SmallAllocator, LargeAllocator>::type
    deallocateAll() {
      SmallAllocator::deallocateAll();
      LargeAllocator::deallocateAll();
    }
  };
}
 
