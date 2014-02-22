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

#include <boost/lockfree/stack.hpp>
#include "ALBAllocatorBase.h"

namespace ALB
{
  /**
   * The SharedFreeList allocator serves a pool of memory blocks and holds them in a list of free blocks
   * Each block's MinSize and MaxSize define the area of blocks sizes that are handled with this allocator.
   * It held until PoolSize blocks. More requested deallocations a forwarded to the Allocator for deallocation.
   * NumberOfBatchAllocations specifies how blocks are allocated by the Allocator. It must be ensured that
   * the Allocator can handle deallocations of sub allocated blocks. (TODO: add static_assert to ensure this
   * at compile time.)
   * MinSize and MaxSize can be set at runtime by instantiating this with ALB::DynamicSetSize.
   * Except the moment of instantiation, this allocator is thread safe and all operations are lock free.
   * \ingroup group_allocators group_shared
   */
  template <class Allocator, int MinSize, int MaxSize, size_t PoolSize = 1024, size_t NumberOfBatchAllocations = 8>
  class SharedFreeList {
    typedef Allocator allocator;
    static const size_t pool_size = PoolSize;

    Allocator _allocator;
    boost::lockfree::stack<void*> _root;

    Helper::Dynastic<(MinSize == DynamicSetSize? DynamicSetSize : MinSize), DynamicSetSize> _lowerBound;
    Helper::Dynastic<(MaxSize == DynamicSetSize? DynamicSetSize : MaxSize), DynamicSetSize> _upperBound;
  
  public:
    SharedFreeList() : _root(PoolSize) {}

    SharedFreeList(size_t minSize, size_t maxSize) 
      : _root(PoolSize) 
    {
      _lowerBound.value(minSize);
      _upperBound.value(maxSize);
    }

    void setMinMax(size_t minSize, size_t maxSize) {
      BOOST_ASSERT_MSG(_lowerBound.value() == -1, 
        "Changing the lower bound during after initialization is not wise!");
      BOOST_ASSERT_MSG(_upperBound.value() == -1,
        "Changing the upper bound during after initialization is not wise!");

      _lowerBound.value(minSize);
      _upperBound.value(maxSize);
    }

    size_t min_size() const {
      return _lowerBound.value();
    }

    size_t max_size() const {
      return _upperBound.value();
    }

    ~SharedFreeList() {
      void* curBlock = nullptr;
      while (_root.pop(curBlock)) {
        _allocator.deallocate(Block(curBlock,_upperBound.value()));
      }
    }

    Block allocate(size_t n) {
      BOOST_ASSERT_MSG(_lowerBound.value() != -1, 
        "The lower bound was not initialized!");
      BOOST_ASSERT_MSG(_upperBound.value() != -1,
        "The upper bound was not initialized!");

      if (_lowerBound.value() <= n && n <= _upperBound.value() && !_root.empty()) {
        void* freeBlock = nullptr;

        if (_root.pop(freeBlock)) {
          return Block(freeBlock, _upperBound.value() - 1);
        }        
        else {
          // allocating in a bunch to gain of having the allocator code in the cache
          size_t blockSize = _upperBound.value();
          auto batchAllocatedBlocks = _allocator.allocate(blockSize * NumberOfBatchAllocations);
          if (batchAllocatedBlocks) {
            // we use the very first block directly so we start at 1
            for (size_t i = 1; i < NumberOfBatchAllocations; i++) {
              if (!_root.push(static_cast<char*>(batchAllocatedBlocks.ptr) + i * blockSize)) {
                BOOST_ASSERT(false);
                _allocator.deallocate(Block(static_cast<char*>(batchAllocatedBlocks.ptr) + i * blockSize, blockSize));
              }
            }
            // returning the first within block
            return Block(batchAllocatedBlocks.ptr, blockSize);
          }
          return _allocator.allocate(blockSize);
        }
      }
      return _allocator.allocate(static_cast<size_t>(_upperBound.value()));
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator::isHandledDefault(*this, b, n)) {
        return true;
      }

      return _allocator.reallocate(b, n);
    }

    bool owns(const Block& block) const {
      return _lowerBound.value() <= block.length && block.length <= _upperBound.value();
    }

    void deallocate(Block& b) {
      if (b && owns(b)) {
        if (_root.push(b.ptr))
        {
          b.reset();
          return;
        }
        _allocator.deallocate(b);
      }
    }
  };
}
