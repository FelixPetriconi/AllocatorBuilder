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
#include <array>

namespace ALB
{
  /**
   * The Bucketizer is intended to hold allocators with StepSize increasing buckets,
   * starting at MinSize until MaxSize.
   * Eg MinSize = 16, MaxSize = 64, StepSize = 16 => 
   *    BucketsSize[16, 33)[32 49)[48 65)
   * It plays very well together with @FreeList or @SharedFreeList
   * After instantiation any instance is as far thread safe as the Allocator is thread
   * safe.
   */
  template <class Allocator, size_t MinSize, size_t MaxSize, size_t StepSize>
  class Bucketizer {
  public:
    static_assert(MinSize < MaxSize, "MinSize must be smaller than MaxSize");

    static const size_t number_of_buckets = (MaxSize - MinSize) / StepSize;    
    Allocator _buckets[number_of_buckets];
    static const size_t max_size = MaxSize;
    static const size_t min_size = MinSize;
    static const size_t step_size = StepSize;
    typename typedef Allocator allocator;

    Bucketizer() {
      for(size_t i = 0; i < number_of_buckets; i++) {
        _buckets[i].setMinMax(MinSize + i * StepSize, MinSize + (i+1) * StepSize + 1 );
      }
    }

    Block allocate(size_t n) {
      size_t i = 0;
      while (i < number_of_buckets) {
        if (_buckets[i].min_size() <= n && n < _buckets[i].max_size()) {
          return _buckets[i]->allocate(n);
        }
        ++i;
      }
      return Block();
    }

    bool owns(const Block& b) {
      return b && (MinSize <= b.length && b.length < MaxSize);
    }
    
    bool reallocate(Block& b, size_t n) {
      BOOST_ASSERT(owns(b));
      BOOST_ASSERT(MinSize <= n && n < MaxSize);

      if (Helper::Reallocator<Bucketizer>::isHandledDefault(*this, b, n)) {
        return true;
      }

      return Helper::reallocateWithCopy(*this, *this, allocator, b, n);
    }

    // Make the function invisible for the has_expand<Bucketizer> trait if the dependent type
    // Allocator does not implement expand
    typename Traits::expand_enabled<Allocator>::type
    expand(Block& b, size_t delta) {
      if (!b) {
        b = allocate(delta);
        return b.length != 0;
      }
      BOOST_ASSERT(owns(b));

      auto it = std::find_if(begin(_buckets), end(_buckets), 
        [&b](const Allocator& a){ return a.owns(b); });

      return it->expand(b);
    }

    void deallocate(Block& b) {
      BOOST_ASSERT(owns(b));
      if (!b || !owns(b)) {
        return;
      }
      auto it = std::find_if(begin(_buckets), end(_buckets), 
        [&b](Allocator& a) { return a.owns(b); });

      (*it)->deallocate(b);
    }

    typename Traits::deallocateAll_enabled<Allocator>::type
    deallocateAll() {
      for (auto& item : _buckets) {
        item.deallocateAll();
      }
    }
  };
}
