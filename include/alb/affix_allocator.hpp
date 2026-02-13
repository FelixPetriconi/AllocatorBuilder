///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#ifndef ALB_AFFIX_ALLOCATOR_HPP
#define ALB_AFFIX_ALLOCATOR_HPP

#include <alb/allocator_base.hpp>
#include <alb/block.hpp>
#include <alb/config.hpp>
#include <alb/internal/affix_helper.hpp>
#include <alb/internal/reallocator.hpp>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {

/**
 * This allocator enables the possibility to surround allocated memory blocks
 * with guards, ref-counter, mutex or etc.
 * It is used by the alb::allocator_with_stats
 * It automatically places an object of type Prefix before the returned memory
 * location and an object of type Suffix after it. In case that they are of type
 * affix_allocatorHelper::Empty nothing is inserted. Depending on the alignment
 * of the used Allocator, Prefix, memory and Suffix are each aligned.
 * Prefix and Suffix, if used, must be trivially copyable. (This cannot be
 * statically asserted, because this would block the possibility to use this
 * allocator_ as guard for memory under- or overflow.
 * One should keep in mind, that using a Suffix is not CPU cache friendly!
 * \tparam Allocator The allocator_ that is used as underlying allocator_
 * \tparam Prefix If defined, then an object of that kind is constructed in
 *                front of any returned block
 * \tparam Suffix If defined, then an object of that kind is constructed beyond
 *               any returned block
 *
 * \ingroup group_allocators group_shared
 */
template <class Allocator, typename Prefix, typename Suffix = affix_helper::no_affix>
class affix_allocator {
    Allocator allocator_;

public:
    using allocator = Allocator;
    using prefix = Prefix;
    using suffix = Suffix;

private:
    static constexpr bool supports_truncated_deallocation =
        Allocator::supports_truncated_deallocation;
    static constexpr unsigned alignment = Allocator::alignment;

    constexpr prefix* inner_to_prefix(const block& b) const noexcept {
        return static_cast<prefix*>(b.ptr);
    }

    constexpr suffix* inner_to_suffix(const block& b) const noexcept {
        return reinterpret_cast<suffix*>(static_cast<std::byte*>(b.ptr) + b.length - suffix_size);
    }

    constexpr block to_inner_block(const block& b) const noexcept {
        return {static_cast<std::byte*>(b.ptr) - prefix_size, b.length + prefix_size + suffix_size};
    }

    constexpr block to_outer_block(const block& b) const noexcept {
        return {static_cast<std::byte*>(b.ptr) + prefix_size, b.length - prefix_size - suffix_size};
    }

public:
    affix_allocator(const affix_allocator&) = delete;
    affix_allocator& operator=(const affix_allocator&) = delete;

    static constexpr std::size_t prefix_size =
        std::is_same_v<Prefix, affix_helper::no_affix> ?
            0 :
            internal::round_to_alignment(alignment, sizeof(prefix));

    static constexpr std::size_t suffix_size =
        std::is_same_v<suffix, affix_helper::no_affix> ?
            0 :
            internal::round_to_alignment(alignment, sizeof(suffix));

    static constexpr std::size_t good_size(std::size_t n) { return allocator::good_size(n); }

    affix_allocator() noexcept {}

    affix_allocator(affix_allocator&& x) noexcept = default;

    affix_allocator& operator=(affix_allocator&& x) noexcept = default;

    /**
     * This Method returns on a given block the prefix.
     * \param b The block that was prefixed. The result is absolute unpredictable
     *          if a block is passed, that is not owned by this allocator_!
     * \return Pointer to the Prefix before the given block
     */
    constexpr prefix* outer_to_prefix(const block& b) const noexcept {
        return b ? reinterpret_cast<prefix*>(static_cast<std::byte*>(b.ptr) - prefix_size) :
                   nullptr;
    }

    /**
     * This Method returns on a given block the suffix.
     * \param b The block that was suffixed. The result is absolute unpredictable
     *          if a block is passed, that is not owned by this allocator_!
     * \return Pointer to the suffix before the given block
     */
    constexpr suffix* outer_to_suffix(const block& b) const noexcept {
        return b ? (reinterpret_cast<suffix*>(static_cast<std::byte*>(b.ptr) + b.length)) : nullptr;
    }

    /**
     * Allocates a Block of n bytes. Actually a Block of n + sizeof(Prefix) +
     * sizeof(Suffix) bytes is allocated. Depending on the defines Prefix and Suffix
     * types objects of this gets instantiated before and/or beyond the returned
     * Block. If Zero bytes are allocated then no allocation at all takes places
     * and an empty Block is returned.
     * \param n Specifies the number of requested bytes. n or more bytes are
     *          returned, depending on the alignment of the underlying Allocator.
     */
    block allocate(size_t n) noexcept {
        block result;
        if (n == 0) {
            return result;
        }

        auto inner_mem = allocator_.allocate(prefix_size + n + suffix_size);
        if (inner_mem) {
            if (prefix_size > 0) {
                affix_helper::create_affix_in_place<prefix>(inner_to_prefix(inner_mem), *this);
            }
            if (suffix_size > 0) {
                affix_helper::create_affix_in_place<suffix>(inner_to_suffix(inner_mem), *this);
            }
            result = to_outer_block(inner_mem);
            return result;
        }
        return result;
    }

    /**
     * The given block gets deallocated. If Prefix or Suffix are defined then
     * their destructors are called.
     * \param b The Block that should be freed.
     */
    void deallocate(block& b) noexcept {
        if (!b) {
            return;
        }
        if constexpr (prefix_size > 0) {
            outer_to_prefix(b)->~prefix();
        }
        if constexpr (suffix_size > 0) {
            outer_to_suffix(b)->~suffix();
        }
        auto innerBlock(to_inner_block(b));
        allocator_.deallocate(innerBlock);
        b.reset();
    }

    /**
     * If the underlying Allocator defines ::owns() this method is available.
     * It returns true, if the given block is owned by this allocator_.
     * \param b The Block that should be checked for ownership
     */
    template <typename U = Allocator>
    std::enable_if_t<traits::has_owns_v<U>, bool> owns(const block& b) const noexcept {
        return b && allocator_.owns(to_inner_block(b));
    }

    /**
     * The given block gets reallocated to the new provided size n. Any potential
     * defined Prefix and/or Suffix gets copied to the new location.
     * \param b The block that should be resized
     * \param n The new size (n = zero means a deallocation)
     * \return True if the operation was successful
     */

    bool reallocate(block& b, std::size_t n) noexcept {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
            return true;
        }
        auto innerBlock = to_inner_block(b);

        // Remember the old Suffix in case that it is available, because it must
        // be later placed to the new position
        affix_helper::optional_suffix_store<Suffix, suffix_size> oldSufix;
        oldSufix.store(outer_to_suffix(b));

        if (allocator_.reallocate(innerBlock, n + prefix_size + suffix_size)) {
            oldSufix.unload(inner_to_suffix(innerBlock));
            b = to_outer_block(innerBlock);
            return true;
        }
        return false;
    }

    /**
     * The method tries to expand the given block by at least delta bytes insito
     * at the given location. This is only available if the underlaying Allocator
     * implements ::expand().
     * \param b The block that should be expanded
     * \param delta The number of bytes that the given block should be increased
     * \return True, if the operation was successful.
     */
    template <typename U = Allocator>
    std::enable_if_t<traits::has_expand_v<U>, bool> expand(block& b,
                                                                    std::size_t delta) noexcept {
        if (delta == 0) {
            return true;
        }

        if (!b) {
            b = allocate(delta);
            return static_cast<bool>(b);
        }

        auto oldBlock = b;
        auto innerBlock = to_inner_block(b);

        if (allocator_.expand(innerBlock, delta)) {
            if constexpr (suffix_size > 0) {
                new (inner_to_suffix(innerBlock)) Suffix(*outer_to_suffix(oldBlock));
            }
            b = to_outer_block(innerBlock);
            return true;
        }
        return false;
    }
};

namespace traits {
/**
 * This trait implements a generic way to access a possible Affix surrounded
 * by a ALB::Block. Normally it returns a nullptr. Only if the passed
 * Allocator is an affix_allocator it returns a real object
 * \ingroup group_traits
 */
template <class Allocator, typename T>
struct affix_extractor {
    static constexpr T* prefix(Allocator&, const block&) noexcept { return nullptr; }
    static constexpr T* suffix(Allocator&, const block&) noexcept { return nullptr; }
};

template <class A, typename Prefix, typename Suffix, typename T>
struct affix_extractor<affix_allocator<A, Prefix, Suffix>, T> {
    static constexpr Prefix* prefix(affix_allocator<A, Prefix, Suffix>& allocator,
                                    const block& b) noexcept {
        return allocator.outer_to_prefix(b);
    }
    static constexpr Suffix* sufix(affix_allocator<A, Prefix, Suffix>& allocator,
                                   const block& b) noexcept {
        return allocator.outer_to_sufix(b);
    }
};
} // namespace traits
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif