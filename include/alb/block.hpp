///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_BLOCK_HPP
#define ALB_BLOCK_HPP

#include <cstdef>
#include <utility>

namespace alb {

inline namespace v_100 {

struct block {
    block() noexcept : ptr(nullptr), length(0) {}

    constexpr block(void* ptr, size_t length) noexcept : ptr(ptr), length(length) {}

    constexpr block(block&& x) noexcept { *this = std::move(x); }

    constexpr block& operator=(block&& x) noexcept {
        ptr = x.ptr;
        length = x.length;
        x.reset();
        return *this;
    }

    constexpr block& operator=(const block& x) noexcept = default;
    constexpr block(const block& x) noexcept = default;

    /**
     * During destruction of any of this instance, the described memory
     * is not freed!
     */
    // ~block()

    /**
     * Clears the block
     */
    constexpr void reset() noexcept {
        ptr = nullptr;
        length = 0;
    }

    /**
     * Bool operator to make the Allocator code better readable
     */
    constexpr explicit operator bool() const { return length != 0; }

    constexpr bool operator==(const block& rhs) const {
        return ptr == rhs.ptr && length == rhs.length;
    }

    /// This points to the start address of the described memory block
    void* ptr;

    /// This describes the length of the reserved bytes.
    std::size_t length;
};

} // namespace v_100
} // namespace alb
#endif
