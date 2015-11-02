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

namespace alb {
  inline namespace v_100 {
    /**
     * This class implements a proxy to the system ::malloc() with the ALB interface.
     * According the template * parameter DefaultAlignment the allocated values are
     * aligned to the specific boundary in bytes. Normally this should be a multiple
     * of at least 4 bytes.
     * \tparam DefaultAlignment Specified the alignment in bytes of all allocation
     *         and reallocations.
     *
     * \ingroup group_allocators group_shared
     */
    template <unsigned DefaultAlignment = 16> class aligned_mallocator {
      static constexpr unsigned int alignment = DefaultAlignment;

      static constexpr size_t good_size(size_t n) {
        return internal::round_to_alignment(alignment, n);
      }

#ifdef _MSC_VER
      bool aligned_reallocate(block &b, size_t n) noexcept
      {
        block reallocatedBlock{ _aligned_realloc(b.ptr, n, alignment), n };

        if (reallocatedBlock) {
          b = reallocatedBlock;
          return true;
        }
        return false;
      }
#else
      // On posix there is no _aligned_realloc so we try a normal realloc
      // if the result is still aligned we are fine
      // otherwise we have to do it by hand
      bool aligned_reallocate(block &b, size_t n) noexcept
      {
        block reallocatedBlock(::realloc(b.ptr, n));
        if (reallocatedBlock) {
          if (static_cast<size_t>(b.ptr) % alignment != 0) {
            auto newAlignedBlock = allocate(n);
            if (!newAlignedBlock) {
              return false;
            }
            internal::block_copy(b, newAlignedBlock);
          }
          else {
            b = reallocatedBlock;
          }

          return true;
        }
        return false;
      }
#endif

    public:
      static constexpr bool supports_truncated_deallocation = false;
      /**
       * Allocates rounded up to the defined alignment the number of bytes.
       * If the system cannot allocate the specified amount of memory then
       * a null Block is returned.
       */
      block allocate(size_t n) noexcept
      {
#ifdef _MSC_VER
        return block{ _aligned_malloc(n, alignment), n };
#else
        return block{ (void *)memalign(alignment, n), n };
#endif
      }

      /**
       * The given block is reallocated to the given size. The new size is aligned
       * to
       * specified alignment. It depends to the OS, if the provided memory block is
       * expanded or moved. It is guaranteed that the values of min(b.length, n)
       * bytes are preserved.
       * \param b The block that should be reallocated
       * \param n The new size of the block
       * \return True, if the operation was successful
       */
      bool reallocate(block &b, size_t n) noexcept
      {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
          return true;
        }

        return aligned_reallocate(b, n);
      }

      /**
       * Frees the given block back to the system. The block gets nulled.
       * \param b The block, describing what memory shall be freed.
       */
      void deallocate(block &b) noexcept
      {
        if (b) {
#ifdef _MSC_VER
          _aligned_free(b.ptr);
#else
          ::free(b.ptr);
#endif
          b.reset();
        }
      }
    };
  }
  using namespace v_100;
}
