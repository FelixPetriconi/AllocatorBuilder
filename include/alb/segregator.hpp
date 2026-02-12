///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_SEGREGATOR_HPP
#define ALB_SEGREGATOR_HPP

#include <alb/allocator_base.hpp>
#include <alb/config.hpp>
#include <alb/internal/reallocator.hpp>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
/**
 * This allocator_ separates the allocation requested depending on a threshold
 * between the Small- and the LargeAllocator
 * \tparam Threshold The edge until all allocations go to the SmallAllocator
 * \tparam SmallAllocator This gets all allocations below the  Threshold
 * \tparam LargeAllocator This gets all allocations starting with the Threshold
 *
 * \ingroup group_allocators group_shared
 */
template <std::size_t Threshold, class SmallAllocator, class LargeAllocator>
class segregator : private SmallAllocator, private LargeAllocator {
    static_assert(!traits::both_same_base<SmallAllocator, LargeAllocator>::value,
                  "Small- and Large-Allocator cannot be both of base!");

public:
    using small_allocator = SmallAllocator;
    using large_allocator = LargeAllocator;

    static constexpr std::size_t threshold = Threshold;

    static constexpr bool supports_truncated_deallocation =
        SmallAllocator::supports_truncated_deallocation &&
        LargeAllocator::supports_truncated_deallocation;

    static constexpr unsigned alignment = (SmallAllocator::alignment > LargeAllocator::alignment) ?
                                              SmallAllocator::alignment :
                                              LargeAllocator::alignment;

    /**
     * Allocates the specified number of bytes. If the operation was not
     * successful
     * it returns an empty block.
     * \param n Number of requested bytes
     * \return Block with the memory information.
     */
    block allocate(std::size_t n) noexcept {
        block result;
        if (n <= Threshold) {
            result = SmallAllocator::allocate(n);

        } else {
            result = LargeAllocator::allocate(n);
        }

        return result;
    }

    /**
     * Frees the given block and resets it.
     * \param b The block to be freed.
     */
    void deallocate(block& b) noexcept {
        if (!b) {
            return;
        }

        if (b.length <= Threshold) {
            return SmallAllocator::deallocate(b);
        }
        return LargeAllocator::deallocate(b);
    }

    /**
     * Reallocates the given block to the given size. If the new size crosses the
     * Threshold, then a memory move will be performed.
     * \param b The block to be changed
     * \param n The new size
     * \return True, if the operation was successful
     *
     * \ingroup group_allocators group_shared
     */
    bool reallocate(block& b, std::size_t n) noexcept {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
            return true;
        }

        if (b.length <= Threshold) {
            if (n <= Threshold) {
                return SmallAllocator::reallocate(b, n);
            }
            return internal::reallocate_with_copy(*this, static_cast<LargeAllocator&>(*this), b, n);
        }

        if (n <= Threshold) {
            return internal::reallocate_with_copy(*this, static_cast<SmallAllocator&>(*this), b, n);
        }
        return LargeAllocator::reallocate(b, n);
    }

    /**
     * The given block will be expanded insito
     * This method is only available if one the Allocators implements it.
     * \param b The block to be expanded
     * \param delta The number of bytes to be expanded
     * \return True, if the operation was successful
     */
    template <typename U = SmallAllocator, typename V = LargeAllocator>
    typename std::enable_if_t<traits::has_expand_v<SmallAllocator> ||
                                  traits::has_expand_v<LargeAllocator>,
                              bool>
        expand(block& b, std::size_t delta) noexcept {
        if (b.length <= Threshold && b.length + delta > Threshold) {
            return false;
        }
        if (b.length <= Threshold) {
            if (traits::has_expand_v<U>) {
                return traits::expander<U>::apply(static_cast<U&>(*this), b, delta);
            }
            return false;
        }
        if (traits::has_expand_v<V>) {
            return traits::expander<V>::apply(static_cast<V&>(*this), b, delta);
        }
        return false;
    }

    /**
     * Checks the ownership of the given block.
     * This is only available if both Allocator implement it
     * \param b The block to checked
     * \return True if one of the allocator_ owns it.
     */
    template <typename U = SmallAllocator, typename V = LargeAllocator>
    typename std::enable_if_t<traits::has_expand_v<SmallAllocator> ||
                                  traits::has_expand_v<LargeAllocator>,
                              bool>
        owns(const block& b) const noexcept {
        if (b.length <= Threshold) {
            return U::owns(b);
        }
        return V::owns(b);
    }

    /**
     * Deallocates all memory.
     * This is available if one of the allocators implement it.
     */
    template <typename U = SmallAllocator, typename V = LargeAllocator>
    typename std::enable_if_t<traits::has_expand_v<SmallAllocator> ||
                                  traits::has_expand_v<LargeAllocator>,
                              void>
        deallocate_all() noexcept {
        traits::all_deallocator<U>::do_it(static_cast<U&>(*this));
        traits::all_deallocator<V>::do_it(static_cast<V&>(*this));
    }
};
} // namespace ALB_VERSION_NAMESPACE()

} // namespace alb

#endif