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

namespace alb {
  /**
   * Allocator that provides memory from the stack.
   * By design it is not thread safe!
   * \tparam MaxSize The maximum number of bytes that can be allocated by this
   *         allocator
   * \tparam Alignment Each memory allocation request by  allocate,
   *         reallocate and expand is aligned by this value
   *
   * \ingroup group_allocators
   */
  template <size_t MaxSize, size_t Alignment = 4> class stack_allocator {
    char _data[MaxSize];
    char *_p;

    bool isLastUsedBlock(const block &b) const noexcept
    {
      return (static_cast<char *>(b.ptr) + b.length == _p);
    }

  public:
    using allocator = stack_allocator;

    static const bool supports_truncated_deallocation = true;
    static const size_t max_size = MaxSize;
    static const size_t alignment = Alignment;

    stack_allocator() noexcept
      : _p(_data)
    {
    }

    block allocate(size_t n) noexcept
    {
      block result;

      if (n == 0) {
        return result;
      }

      const auto alignedLength = internal::roundToAlignment(Alignment, n);
      if (alignedLength + _p > _data + MaxSize) { // not enough memory left
        return result;
      }

      result.ptr = _p;
      result.length = alignedLength;

      _p += alignedLength;
      return result;
    }

    void deallocate(block &b) noexcept
    {
      if (!b) {
        return;
      }
      if (!owns(b)) {
        assert(false);
        return;
      }

      // If it was the most recent allocated MemoryBlock, then we can re-use the
      // memory. Otherwise this freed MemoryBlock is not available for further
      // allocations. Since all happens on the stack this is not a leak!
      if (isLastUsedBlock(b)) {
        _p = static_cast<char *>(b.ptr);
      }
      b.reset();
    }

    bool reallocate(block &b, size_t n) noexcept
    {
      if (b.length == n) {
        return true;
      }

      if (n == 0) {
        deallocate(b);
        return true;
      }

      if (!b) {
        b = allocate(n);
        return true;
      }

      const auto alignedLength = internal::roundToAlignment(Alignment, n);

      if (isLastUsedBlock(b)) {
        if (static_cast<char *>(b.ptr) + alignedLength <= _data + MaxSize) {
          b.length = alignedLength;
          _p = static_cast<char *>(b.ptr) + alignedLength;
          return true;
        }
        // out of memory
        return false;
      }
      if (b.length > n) {
        b.length = internal::roundToAlignment(Alignment, n);
        return true;
      }

      auto newBlock = allocate(alignedLength);
      // we cannot deallocate the old block, because it is in between used ones,
      //  so we have to "leak" here.
      if (newBlock) {
        internal::blockCopy(b, newBlock);
        b = newBlock;
        return true;
      }
      return false;
    }

    /**
     * Expands the given block insito by the amount of bytes
     * \param b The block that should be expanded
     * \param delta The amount of bytes that should be appended
     * \return true, if the operation was successful or false if not enough
     *         memory is left
     */
    bool expand(block &b, size_t delta) noexcept
    {
      if (delta == 0) {
        return true;
      }
      if (!b) {
        b = allocate(delta);
        return b.length != 0;
      }
      if (!isLastUsedBlock(b)) {
        return false;
      }
      auto alignedBytes = internal::roundToAlignment(Alignment, delta);
      if (_p + alignedBytes > _data + MaxSize) {
        return false;
      }
      _p += alignedBytes;
      b.length += alignedBytes;
      return true;
    }

    /**
     * Returns true, if the provided block was allocated previously with this
     * allocator
     * \param b The block to be checked.
     */
    bool owns(const block &b) const noexcept
    {
      return b && (b.ptr >= _data && b.ptr < _data + MaxSize);
    }

    /**
     * Sets all possibly provided memory to free.
     * Be warned that all usage of previously allocated blocks results in
     * unpredictable results!
     */
    void deallocateAll() noexcept
    {
      _p = _data;
    }

  private:
    // disable move ctor and move assignment operators
    stack_allocator(stack_allocator &&) = delete;
    stack_allocator &operator=(stack_allocator &&) = delete;
    stack_allocator(const stack_allocator &) = delete;
    stack_allocator &operator=(const stack_allocator &) = delete;
    // disable heap allocation
    void *operator new(size_t) = delete;
    void *operator new[](size_t) = delete;
    void operator delete(void *) = delete;
    void operator delete[](void *) = delete;
    // disable address taking
    stack_allocator *operator&() = delete;
  };

  template <size_t MaxSize, size_t Alignment>
  const size_t stack_allocator<MaxSize, Alignment>::max_size;
  template <size_t MaxSize, size_t Alignment>
  const size_t stack_allocator<MaxSize, Alignment>::alignment;
}
