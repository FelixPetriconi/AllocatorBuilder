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

#include "internal/traits.hpp"
#include <utility>
#include <type_traits>
#include <stddef.h>

namespace alb {

  /**
   * The value type to describe a memory block and it's length
   * \ingroup group_allocators
   */
  struct block {
    block() noexcept
      : ptr(nullptr)
      , length(0)
    {
    }

    block(void *ptr, size_t length) noexcept
      : ptr(ptr)
      , length(length)
    {
    }

    block(block &&x) noexcept
    {
      *this = std::move(x);
    }

    block &operator=(block &&x) noexcept
    {
      ptr = x.ptr;
      length = x.length;
      x.reset();
      return *this;
    }

    block &operator=(const block &x) noexcept = default;
    block(const block &x) noexcept = default;

    /**
     * During destruction of any of this instance, the described memory
     * is not freed!
     */
    ~block()
    {
    }

    /**
     * Clears the block
     */
    void reset() noexcept
    {
      ptr = nullptr;
      length = 0;
    }

    /**
     * Bool operator to make the Allocator code better readable
     */
    explicit operator bool() const
    {
      return length != 0;
    }

    bool operator==(const block &rhs) const
    {
      return ptr == rhs.ptr && length == rhs.length;
    }

    /// This points to the start address of the described memory block
    void *ptr;

    /// This describes the length of the reserved bytes.
    size_t length;
  };

  namespace internal {

    /**
     * Copies std::min(source.length, destination.length) bytes from source to
     * destination
     *
     * \ingroup group_internal
     */
    void blockCopy(const block &source, block &destination) noexcept;

    /**
     * Returns a upper rounded value of multiples of a
     * \ingroup group_internal
     */
    inline size_t roundToAlignment(size_t basis, size_t n) noexcept
    {
      auto remainder = n % basis;
      return n + ((remainder == 0) ? 0 : (basis - remainder));
    }

  } // namespace Helper
}
