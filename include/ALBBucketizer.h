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
   * within the range of [MinSize, MaxSize)
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
    static_assert((MaxSize - MinSize + 1) % StepSize == 0, "Incorrect ranges or step size!");

    static const size_t number_of_buckets = (MaxSize - MinSize + 1) / StepSize;    
    Allocator _buckets[number_of_buckets];
    static const size_t max_size = MaxSize;
    static const size_t min_size = MinSize;
    static const size_t step_size = StepSize;
    typename typedef Allocator allocator;

    Bucketizer() {
      for(size_t i = 0; i < number_of_buckets; i++) {
        _buckets[i].setMinMax(MinSize + i * StepSize, MinSize + (i+1) * StepSize - 1 );
      }
    }

    Block allocate(size_t n) {
      size_t i = 0;
      while (i < number_of_buckets) {
        if (_buckets[i].min_size() <= n && n <= _buckets[i].max_size()) {
          return _buckets[i].allocate(n);
        }
        ++i;
      }
      return Block();
    }

    bool owns(const Block& b) {
      return b && (MinSize <= b.length && b.length <= MaxSize);
    }
    
    bool reallocate(Block& b, size_t n) {
      if (n != 0 && (n < MinSize || n > MaxSize) ) {
        return false;
      }

      if (Helper::Reallocator<Bucketizer>::isHandledDefault(*this, b, n)) {
        return true;
      }

      BOOST_ASSERT(owns(b));

      const auto alignedLength = Helper::roundToAlignment(StepSize, n);
      auto currentAllocator = findMatchingAllocator(b.length);
      auto newAllocator = findMatchingAllocator(alignedLength);

      if (currentAllocator == newAllocator) {
        return true;
      }

      return Helper::reallocateWithCopy(*currentAllocator, *newAllocator, b, alignedLength);
    }

    void deallocate(Block& b) {
      if (!b) {
        return;
      }    
      BOOST_ASSERT_MSG(owns(b), "It is not wise to let me deallocate a foreign Block!");
      if (!owns(b)) {
        return;
      }
      
      auto currentAllocator = findMatchingAllocator(b.length);
      currentAllocator->deallocate(b);
    }

    typename Traits::deallocateAll_enabled<Allocator>::type
    deallocateAll() {
      for (auto& item : _buckets) {
        item.deallocateAll();
      }
    }
  private:
    
    Allocator* findMatchingAllocator(size_t n) {
      BOOST_ASSERT(MinSize <= n && n <= MaxSize);
      auto v = ALB::Helper::roundToAlignment(StepSize, n);
      return &_buckets[(v - MinSize)/StepSize];
    }
  };
}
