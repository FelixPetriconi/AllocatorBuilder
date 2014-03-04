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
#include "ALBHelperStack.h"
#include <boost/lockfree/stack.hpp>
#include <boost/config/suffix.hpp>


namespace ALB {
/**
 * The FreeListBase allocator is a generic implementation of a free list pool
 * Users shall use the ALB::FreeList or the ALB::SharedFreeList.
 * This class serves a pool of memory blocks and holds them
 * in a list of free blocks Each block's MinSize and MaxSize define the area
 * of blocks sizes that are handled with this allocator.
 * It held until PoolSize blocks. More requested deallocations a forwarded to
 * the Allocator for deallocation.
 * NumberOfBatchAllocations specifies how blocks are allocated by the Allocator.
 * MinSize and MaxSize can be set at runtime by instantiating this with
 * ALB::DynasticDynamicSet.
 * Except the moment of instantiation, this allocator is thread safe and all
 * operations are lock free.
 * \tparam Shared Set to true, for a multi threaded usage, otherwise to false
 * \tparam Allocator Then allocator that should be used, when a new resource is
 *                    needed
 *
 * \ingroup group_allocators group_shared
 */
template <bool Shared, class Allocator, size_t MinSize, size_t MaxSize,
          unsigned PoolSize, unsigned NumberOfBatchAllocations>
class FreeListBase {
  Allocator _allocator;

  typename traits::type_switch<
      boost::lockfree::stack<void *, boost::lockfree::fixed_sized<true>,
                             boost::lockfree::capacity<PoolSize> >,
      Helper::stack<void *, PoolSize>, Shared>::type _root;

  Helper::Dynastic<(MinSize == DynasticDynamicSet ? DynasticDynamicSet : MinSize),
                   DynasticDynamicSet> _lowerBound;
  Helper::Dynastic<(MaxSize == DynasticDynamicSet ? DynasticDynamicSet : MaxSize),
                   DynasticDynamicSet> _upperBound;

public:
  typedef Allocator allocator;
  BOOST_STATIC_CONSTANT(unsigned, pool_size = PoolSize);
  
  BOOST_STATIC_CONSTANT(unsigned, number_of_batch_allocations = NumberOfBatchAllocations);

  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation =
      Allocator::supports_truncated_deallocation);

  FreeListBase() {}

  /**
   * Constructs a FreeListBase with the specified bounding edges
   * This c'tor is just available if the template parameter MinSize
   * and MaxSize are set to DynasticDynamicSet. (Otherwise the compiler
   * will tell ;-)
   * \param minSize The lower boundary accepted by this Allocator
   * \param maxSize The upper boundary accepted by this Allocator
   */
  FreeListBase(size_t minSize, size_t maxSize) {
    _lowerBound.value(minSize);
    _upperBound.value(maxSize);
  }

  /**
   * Set the min and max boundary of this allocator. This method is
   * just available if the template parameter MinSize and MaxSize
   * are set to DynasticDynamicSet. (Otherwise the compiler will tell ;-)
   * \param minSize The lower boundary accepted by this Allocator
   * \param maxSize The upper boundary accepted by this Allocator
   */
  void setMinMax(size_t minSize, size_t maxSize) {
    BOOST_ASSERT_MSG(
      _lowerBound.value() == DynasticUndefined,
        "Changing the lower bound during after initialization is not wise!");
    BOOST_ASSERT_MSG(
      _upperBound.value() == DynasticUndefined,
        "Changing the upper bound during after initialization is not wise!");

    _lowerBound.value(minSize);
    _upperBound.value(maxSize);
  }

  /**
   * Returns the lower boundary
   */
  size_t min_size() const { return _lowerBound.value(); }

  /**
   * Returns the upper boundary
   */
  size_t max_size() const { return _upperBound.value(); }

  /**
   * Frees all resources. Beware of using allocated blocks given by
   * this allocator after calling this.
   */
  ~FreeListBase() {
    void *curBlock = nullptr;
    while (_root.pop(curBlock)) {
      Block oldBlock(curBlock, _upperBound.value());
      _allocator.deallocate(oldBlock);
    }
  }

  /**
   * Provides a block. If it is available in the pool, then this will be
   * reused. If the pool is empty, then a new block will be created and
   * returned. The passed size n must be within the boundary of the
   * allocator, otherwise an empty block will returned.
   * Depending on the parameter NumberOfBatchAllocations not only one new
   * block is allocated, but as many as specified.
   * \param n The number of requested bytes. The result is aligned to the
   *          upper boundary.
   * \return The allocated block
   */
  Block allocate(size_t n) {
    BOOST_ASSERT_MSG(_lowerBound.value() != -1,
                     "The lower bound was not initialized!");
    BOOST_ASSERT_MSG(_upperBound.value() != -1,
                     "The upper bound was not initialized!");

    if (_lowerBound.value() <= n && n <= _upperBound.value()) {
      void *freeBlock = nullptr;

      if (_root.pop(freeBlock)) {
        return Block(freeBlock, _upperBound.value());
      } else {
        size_t blockSize = _upperBound.value();
        if (supports_truncated_deallocation) {
          // allocating in a bunch to gain of having the allocator code in the
          // cache
          auto batchAllocatedBlocks =
              _allocator.allocate(blockSize * NumberOfBatchAllocations);

          if (batchAllocatedBlocks) {
            // we use the very first block directly so we start at 1
            for (size_t i = 1; i < NumberOfBatchAllocations; i++) {
              if (!_root.push(static_cast<char *>(batchAllocatedBlocks.ptr) +
                              i * blockSize)) {
                BOOST_ASSERT(false);
                ALB::Block oldBlock(static_cast<char *>(batchAllocatedBlocks.ptr) +
                              i * blockSize,
                          blockSize);
                _allocator.deallocate(oldBlock);
              }
            }
            // returning the first within block
            return Block(batchAllocatedBlocks.ptr, blockSize);
          }
          return _allocator.allocate(blockSize);
        } else {
          for (size_t i = 0; i < NumberOfBatchAllocations - 1; i++) {
            auto b = _allocator.allocate(blockSize);
            if (!_root.push(b.ptr)) { // the list is full in the meantime, so we
                                      // exit early
              return b;
            }
          }
          return _allocator.allocate(blockSize);
        }
      }
    }
    return Block();
  }

  /**
   * Reallocates the given block. In this case only trivial case can lead to
   * a positive result. In general reallocation to a different size > 0 is not
   * supported by this allocator.
   * \param b The block to reallocate
   * \param n The new size
   * \return True, if the reallocation was successful.
   */
  bool reallocate(Block &b, size_t n) {
    if (Helper::Reallocator<decltype(*this)>::isHandledDefault(*this, b, n)) {
      return true;
    }
    return false;
  }

  /**
   * Checks the ownership of the given block
   * \param b The block to check
   * \return True, it is owned by this allocator
   */
  bool owns(const Block &b) const {
    return b && _lowerBound.value() <= b.length &&
           b.length <= _upperBound.value();
  }

  /**
   * Appends the block to the free list if it is not filled up. The given block
   * is reset
   * \param b The block to free
   */
  void deallocate(Block &b) {
    if (b && owns(b)) {
      if (_root.push(b.ptr)) {
        b.reset();
        return;
      }
      _allocator.deallocate(b);
    }
  }
};

/**
 * This class is a thread safe specialization of the FreeList. For details see
 * ALB::FreeListBase
 *
 * \ingroup group_allocator group_shared
 */
template <class Allocator, size_t MinSize, size_t MaxSize, size_t PoolSize = 1024,
          size_t NumberOfBatchAllocations = 8>
class SharedFreeList : public FreeListBase<true, Allocator, MinSize, MaxSize,
                                           PoolSize, NumberOfBatchAllocations> {
public:
  SharedFreeList()
    : FreeListBase<true, Allocator, MinSize, MaxSize,
                   PoolSize, NumberOfBatchAllocations>() {}

  SharedFreeList(size_t minSize, size_t maxSize)
    : FreeListBase<true, Allocator, MinSize, MaxSize,
                   PoolSize, NumberOfBatchAllocations>(minSize, maxSize) {}
};

/**
* This class is a single threaded specialization of the FreeList. For details
* see ALB::FreeListBase
*
* \ingroup group_allocator
*/
template <class Allocator, size_t MinSize, size_t MaxSize, size_t PoolSize = 1024,
          size_t NumberOfBatchAllocations = 8>
class FreeList : public FreeListBase<false, Allocator, MinSize, MaxSize,
                                     PoolSize, NumberOfBatchAllocations> {
public:
  FreeList() 
    : FreeListBase<false, Allocator, MinSize, MaxSize,
                   PoolSize, NumberOfBatchAllocations>() {}

  FreeList(size_t minSize, size_t maxSize) 
    : FreeListBase<false, Allocator, MinSize, MaxSize,
                   PoolSize, NumberOfBatchAllocations>(minSize, maxSize) {}

};
}
