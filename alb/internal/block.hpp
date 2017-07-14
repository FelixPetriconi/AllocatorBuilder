#ifndef BLOCK_HPP
#define BLOCK_HPP

#include <utility>

namespace alb {
  inline namespace v_100 {

    struct block {
      block() noexcept
        : ptr(nullptr)
        , length(0)
      {
      }

      constexpr block(void *ptr, size_t length) noexcept
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
      // ~block()
      // {
      // }

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

  }
}
#endif
