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
#include <cassert>

namespace alb {
  /**
   * The Bucketizer is intended to hold allocators with StepSize increasing
   * buckets,
   * within the range of [MinSize, MaxSize)
   * E.g. MinSize = 17, MaxSize = 64, StepSize = 16 =>
   *      BucketsSize[17, 32][33 48][48 64]
   * It plays very well together with alb::freelist or alb::shared_freelist
   * After instantiation any instance is as far thread safe as the Allocator is
   * thread
   * safe.
   * \tparam Allocator Specifies which shall be handled in a bucketized way
   * \tparam MinSize The minimum size of the first bucket item
   * \tparam MaxSize The upper size of the last bucket item
   * \tparam StepSize The equidistant step size of the size of all buckets
   *
   * \ingroup group_allocators group_shared
   */
  template <class Allocator, unsigned MinSize, unsigned MaxSize, unsigned StepSize>
  class bucketizer {
  public:
    static const bool supports_truncated_deallocation = false;

    static_assert(MinSize < MaxSize, "MinSize must be smaller than MaxSize");
    static_assert((MaxSize - MinSize + 1) % StepSize == 0, "Incorrect ranges or step size!");

    static const unsigned number_of_buckets = ((MaxSize - MinSize + 1) / StepSize);
    static const unsigned max_size = MaxSize;
    static const unsigned min_size = MinSize;
    static const unsigned step_size = StepSize;

    using allocator = Allocator;

    Allocator _buckets[number_of_buckets];

    bucketizer() noexcept
    {
      for (size_t i = 0; i < number_of_buckets; i++) {
        _buckets[i].setMinMax(MinSize + i * StepSize, MinSize + (i + 1) * StepSize - 1);
      }
    }

    /**
     * Allocates the requested number of bytes. The request is forwarded to
     * the bucket with which edges are at [min,max] bytes.
     * \param n The number of bytes to be allocated
     * \return The Block describing the allocated memory
     */
    block allocate(size_t n) noexcept
    {
      size_t i = 0;
      while (i < number_of_buckets) {
        if (_buckets[i].min_size() <= n && n <= _buckets[i].max_size()) {
          return _buckets[i].allocate(n);
        }
        ++i;
      }
      return {};
    }

    /**
     * Checks, if the given block is owned by one of the bucket item
     * \param b The block to be checked
     * \return Returns true, if the block is owned by one of the bucket items
     */
    bool owns(const block &b) const noexcept
    {
      return b && (MinSize <= b.length && b.length <= MaxSize);
    }

    /**
     * Forwards the reallocation of the given block to the corresponding bucket
     * item.
     * If the length of the given block and the specified new size crosses the
     * boundary of a bucket, then content memory of the block is moved to the
     * new bucket item
     * \param b Then  Block its size should be changed
     * \param n The new size of the block.
     * \return True, if the reallocation was successful.
     */
    bool reallocate(block &b, size_t n) noexcept
    {
      if (n != 0 && (n < MinSize || n > MaxSize)) {
        return false;
      }

      if (internal::reallocator<bucketizer>::isHandledDefault(*this, b, n)) {
        return true;
      }

      assert(owns(b));

      const auto alignedLength = internal::roundToAlignment(StepSize, n);
      auto currentAllocator = findMatchingAllocator(b.length);
      auto newAllocator = findMatchingAllocator(alignedLength);

      if (currentAllocator == newAllocator) {
        return true;
      }

      return internal::reallocateWithCopy(*currentAllocator, *newAllocator, b, alignedLength);
    }

    /**
     * Frees the given block and resets it.
     * \param b The block, its memory should be freed
     */
    void deallocate(block &b) noexcept
    {
      if (!b) {
        return;
      }
      if (!owns(b)) {
        assert(!"It is not wise to let me deallocate a foreign Block!");
        return;
      }

      auto currentAllocator = findMatchingAllocator(b.length);
      currentAllocator->deallocate(b);
    }

    /**
     * Deallocates all resources. Beware of possible dangling pointers!
     * This method is only available if Allocator::deallocateAll is available
     */
    template <typename U = Allocator>
    typename std::enable_if<traits::has_deallocateAll<U>::value, void>::type
    deallocateAll() noexcept
    {
      for (auto &item : _buckets) {
        traits::AllDeallocator<U>::doIt(item);
      }
    }

  private:
    Allocator *findMatchingAllocator(size_t n) noexcept
    {
      assert(MinSize <= n && n <= MaxSize);
      auto v = alb::internal::roundToAlignment(StepSize, n);
      return &_buckets[(v - MinSize) / StepSize];
    }
  };

  template <class Allocator, unsigned MinSize, unsigned MaxSize, unsigned StepSize>
  const unsigned bucketizer<Allocator, MinSize, MaxSize, StepSize>::number_of_buckets;
}
