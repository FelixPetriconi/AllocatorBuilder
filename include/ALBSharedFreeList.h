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
   * NumberOfBatchAllocations specifies how blocks are allocated by the Allocator. 
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
    static const bool supports_truncated_deallocation = Allocator::supports_truncated_deallocation;
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

      if (_lowerBound.value() <= n && n <= _upperBound.value()) {
        void* freeBlock = nullptr;

        if (_root.pop(freeBlock)) {
          return Block(freeBlock, _upperBound.value());
        }        
        else {
          size_t blockSize = _upperBound.value();
          if (supports_truncated_deallocation) {
            // allocating in a bunch to gain of having the allocator code in the cache
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
          else {
            for (size_t i = 0; i < NumberOfBatchAllocations - 1; i++) {
              auto b = _allocator.allocate(blockSize);
              if (!_root.push(b.ptr)) { // the list is full in the meantime, so we exit early
                return b;
              }
            }
            return _allocator.allocate(blockSize);
          }
        }
      }
      return Block();
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<SharedFreeList>::isHandledDefault(*this, b, n)) {
        return true;
      }

      return false;
    }

    /**
     * Checks the ownership of the given block
     * @param b The block to check
     * @return True, it is owned by this allocator
     */
    bool owns(const Block& b) const {
      return b && _lowerBound.value() <= b.length && b.length <= _upperBound.value();
    }

    /**
     * Appends the block to the free list if it is not filled up. The given block is reset
     * @param b The block to free
     */
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
