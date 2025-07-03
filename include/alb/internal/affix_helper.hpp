///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_AFFIX_HELPER_HPP
#define ALB_AFFIX_HELPER_HPP

#include <cstdint>
#include <type_traits>

namespace alb {
inline namespace v_100 {
namespace affix_helper {

template <typename Affix, typename Enabled = void>
struct affix_creator;

template <typename Affix>
struct affix_creator<Affix,
                     typename std::enable_if_t<std::is_default_constructible_v<Affix>>> {
    template <typename Allocator>
    static constexpr void create(void* p, Allocator&) {
        new (p) Affix{};
    }
};

template <typename Affix>
struct affix_creator<Affix,
                     typename std::enable_if_t<!std::is_default_constructible_v<Affix>>> {
    template <typename Allocator>
    static constexpr void create(void* p, Allocator& a) {
        new (p) Affix(a);
    }
};

template <typename Affix, typename Allocator>
void create_affix_in_place(void* p, Allocator& a) {
    affix_creator<Affix>::create(p, a);
}

/**
 * This special kind of no_affix is necessary to have the possibility to test
 * the affix_allocator in a generic way. This must be used to disable a Prefix or
 * Suffix.
 */
struct no_affix {
    using value_type = std::int32_t;
    static const std::int32_t pattern = 0;
};

/* simple optional store of a Suffix if the suffix_size > 0 */
template <typename Suffix, std::size_t>
struct optional_suffix_store {
    Suffix o_;
    void store(Suffix* o) noexcept { o_ = *o; }
    void unload(Suffix* o) noexcept { new (o) Suffix(o_); }
};

template <typename Suffix>
struct optional_suffix_store<Suffix, 0> {
    void store(Suffix*) {}
    void unload(Suffix*) {}
};

} // namespace affix_helper
} // namespace v_100
using namespace v_100;

} // namespace alb

#endif