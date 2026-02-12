///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_MALLOCATOR_HPP
#define ALB_MALLOCATOR_HPP

#include <alb/allocator_base.hpp>
#include <alb/config.hpp>
#include <alb/internal/reallocator.hpp>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
/**
 * This class implements a facade against the system ::malloc()
 *
 * \ingroup group_allocators group_shared
 */
class mallocator {
public:
    static constexpr bool supports_truncated_deallocation = false;
    static constexpr std::uint32_t alignment = 4;

    /**
     * Allocates the specified number of bytes.
     * If the system cannot allocate the specified amount of memory then
     * a null Block is returned.
     * \param n The number of bytes.
     * \return Block with memory information
     */
    block allocate(std::size_t n) noexcept {
        block result;

        if (n == 0) {
            return result;
        }
        auto p = ::malloc(n);
        if (p != nullptr) {
            result.ptr = p;
            result.length = n;
            return result;
        }
        return result;
    }

    /**
     * Reallocate the specified block to the specified size.
     * \param b The block to be reallocated
     * \param n The new size
     * \return True, if the operation was successful.
     */
    bool reallocate(block& b, std::size_t n) noexcept {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
            return true;
        }

        block reallocatedBlock(::realloc(b.ptr, n), n);

        if (reallocatedBlock) {
            b = reallocatedBlock;
            return true;
        }
        return false;
    }

    /**
     * Frees the given block and resets it.
     * \param b Block to be freed.
     */
    void deallocate(block& b) noexcept {
        if (b) {
            ::free(b.ptr);
            b.reset();
        }
    }
};
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif