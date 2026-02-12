///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_FALLBACK_ALLOCATOR_HPP
#define ALB_FALLBACK_ALLOCATOR_HPP

#include <alb/allocator_base.hpp>
#include <alb/config.hpp>
#include <alb/internal/reallocator.hpp>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
/**
 * All allocation requests are passed to the Primary allocator_. Only if this
 * cannot fulfill the request, it is passed to the Fallback allocator_
 * \tparam Primary The allocator_ that gets all requests by default
 * \tparam Fallback The allocator_ that get the requests, if the Primary failed.
 *
 * \ingroup group_allocators group_shared
 */
template <class Primary, class Fallback>
class fallback_allocator : public Primary, public Fallback {
    using primary = Primary;
    using fallback = Fallback;

    static_assert(!traits::both_same_base<primary, fallback>::value,
                  "Primary- and Fallback-Allocator cannot be both of the same base!");

public:
    static constexpr bool supports_truncated_deallocation =
        primary::supports_truncated_deallocation || fallback::supports_truncated_deallocation;

    static constexpr unsigned alignment =
        (primary::alignment > fallback::alignment) ? primary::alignment : fallback::alignment;

    /**
     * Allocates the requested number of bytes.
     * \param n The number of bytes. Depending on the alignment of the allocator_,
     *          the block might contain a bigger size
     */
    block allocate(size_t n) noexcept {
        block result;
        if (n == 0) {
            return result;
        }
        result = primary::allocate(n);
        if (!result) result = fallback::allocate(n);

        return result;
    }

    /**
     * Frees the memory of the provided block and resets it.
     * \param b The block describing the memory to be freed.
     */
    void deallocate(block& b) noexcept {
        if (!b) {
            return;
        }

        if (primary::owns(b))
            primary::deallocate(b);
        else
            fallback::deallocate(b);
    }

    /**
     * Reallocates the given block to the given size. If the Primary cannot handle
     * the request, then a memory move is done.
     * \param b The block to be reallocated
     * \param n The new size (Zero means deallocation.)
     * \return True if the operation was successful
     */
    bool reallocate(block& b, size_t n) noexcept {
        if (primary::owns(b)) {
            if (internal::is_reallocation_handled_default(static_cast<primary&>(*this), b, n)) {
                return true;
            }
        } else {
            if (internal::is_reallocation_handled_default(static_cast<fallback&>(*this), b, n)) {
                return true;
            }
        }

        if (primary::owns(b)) {
            if (primary::reallocate(b, n)) {
                return true;
            }
            return internal::reallocate_with_copy(static_cast<primary&>(*this),
                                                  static_cast<fallback&>(*this), b, n);
        }

        return fallback::reallocate(b, n);
    }

    /**
     * Expands the given block by the given amount of bytes.
     * This method is only available if at least one of the allocators implements
     * it
     * \param b The block that should be expanded
     * \param delta The number of bytes that should be appended
     * \return True, if the operation could be performed successful.
     */
    template <typename U = Primary, typename V = Fallback>
    typename std::enable_if_t<traits::has_expand_v<U> || traits::has_expand_v<V>, bool> expand(
        block& b, size_t delta) noexcept {
        if (primary::owns(b)) {
            if constexpr (traits::has_expand_v<U>) {
                return traits::expander<U>::apply(static_cast<U&>(*this), b, delta);
            }
            return false;
        }
        if constexpr (traits::has_expand_v<V>) {
            return traits::expander<V>::apply(static_cast<V&>(*this), b, delta);
        }
        return false;
    }

    /**
     * Checks for the ownership of the given block
     * This method is only available, if both allocators implements it.
     * \param b The block which ownership shall be checked.
     * \return True if the block comes from one of the allocators.
     */
    template <typename U = Primary, typename V = Fallback>
    typename std::enable_if_t<traits::has_owns_v<U> && traits::has_owns_v<V>, bool> owns(
        const block& b) const noexcept {
        return primary::owns(b) || fallback::owns(b);
    }

    template <typename U = Primary, typename V = Fallback>
    typename std::enable_if_t<traits::has_deallocate_all_v<U> && traits::has_deallocate_all_v<V>,
                              void>
        deallocate_all() noexcept {
        primary::deallocate_all();
        fallback::deallocate_all();
    }
};
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif