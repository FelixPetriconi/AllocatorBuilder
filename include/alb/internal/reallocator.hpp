///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_REALLOCATOR_HPP
#define ALB_REALLOCATOR_HPP

#include <alb/block.hpp>
#include <alb/config.hpp>
#include <alb/internal/traits.hpp>
#include <type_traits>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
namespace internal {

/**
 * Allocates a new block of n bytes with newAllocator, copies min(b.length, n)
 * bytes to it, deallocates the old block b, and returns the new block.
 * \tparam OldAllocator The allocator_ that allocated the passed block
 * \tparam NewAllocator The allocator_ that should be used for the new allocation
 * \param old_allocator The instance of the allocator_ that allocated in the past
 *                     the block. The behavior is undefined if the block was
 *                     not allocated by it!
 * \param new_allocator The instance that should be used for the new allocation
 * \param b The block that should be reallocated by a move of its content
 * \param n The new size of the block
 * \return True, if the operation was successful
 *
 * \ingroup group_internal
 */
template <class OldAllocator, class NewAllocator>
bool reallocate_with_copy(OldAllocator& old_allocator,
                          NewAllocator& new_allocator,
                          block& b,
                          size_t n) noexcept {
    auto newBlock = new_allocator.allocate(n);
    if (!newBlock) {
        return false;
    }
    block_copy(b, newBlock);
    old_allocator.deallocate(b);
    b = newBlock;
    return true;
}

/**
 * The Reallocator handles standard use cases during the deallocation.
 * If available it uses ::expand() of the allocator_.
 * (With C++11 this could be done with a partial specialized function,
 * but VS 2012 does not support this.)
 * \tparam Allocator The allocator_ that should be used during the reallocation
 *
 * \ingroup group_internal
 */
template <class Allocator, typename Enabled = void>
struct reallocator;

/**
 * Specialization for Allocators that implements Allocator::expand()
 * \tparam Allocator The allocator_ that should be used during the reallocation
 *
 * \ingroup group_internal
 */
template <class Allocator>
struct reallocator<Allocator, std::enable_if_t<traits::has_expand_v<Allocator>>> {
    static bool is_handled_default(Allocator& allocator, block& b, std::size_t n) noexcept {
        if (b.length == n) {
            return true;
        }
        if (n == 0) {
            allocator.deallocate(b);
            return true;
        }
        if (!b) {
            b = allocator.allocate(n);
            return true;
        }
        if (n > b.length) {
            if (allocator.expand(b, n - b.length)) {
                return true;
            }
        }
        return false;
    }
};

/**
 * Specialization for Allocators, that don't implement Allocator::expand()
 * \tparam Allocator The allocator_ that should be used during the reallocation
 *
 * \ingroup group_internal
 */
template <class Allocator>
struct reallocator<Allocator, std::enable_if_t<!traits::has_expand_v<Allocator>>> {
    static bool is_handled_default(Allocator& allocator, block& b, std::size_t n) noexcept {
        if (b.length == n) {
            return true;
        }
        if (n == 0) {
            allocator.deallocate(b);
            return true;
        }
        if (!b) {
            b = allocator.allocate(n);
            return true;
        }
        return false;
    }
};

template <typename Allocator>
bool is_reallocation_handled_default(Allocator& allocator, block& b, std::size_t n) noexcept {
    return reallocator<Allocator>::is_handled_default(allocator, b, n);
}
} // namespace internal
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif