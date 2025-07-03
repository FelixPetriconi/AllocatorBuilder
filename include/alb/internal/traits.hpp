///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_TRAITS_HPP
#define ALB_TRAITS_HPP

#include <alb/block.hpp>

#include <cstdint>
#include <type_traits>

namespace alb {

inline namespace v_100 {

namespace traits {

// implementation of the detect idiom as in std::experimental

template <class...>
using void_t = void;

struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const&) = delete;
    void operator=(nonesuch const&) = delete;
};

namespace detail {
template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

template <class Default, template <class...> class Op, class... Args>
struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};

} // namespace detail

template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

template <class Default, template <class...> class Op, class... Args>
using detected_or = detail::detector<Default, void, Op, Args...>;

template <class Expected, template <class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

/**
 * Trait that checks if the given class implements bool expand(Block&, size_t)
 *
 * \ingroup group_traits
 */

template <class T>
using has_expand =
    decltype(std::declval<T&>().expand(std::declval<block>(), std::declval<std::size_t>()));

template <typename T>
using has_expand_v = is_detected_exact<bool, has_expand, T>;

/**
 * Trait that checks if the given class implements void deallocate_all()
 *
 * \ingroup group_traits
 */

template <class T>
using has_deallocate_all = decltype(std::declval<T&>().deallocate_all());

template <typename T>
using has_deallocate_all_v = is_detected_exact<void, has_deallocate_all, T>;

/**
 * Trait that checks if the given class implements bool owns(const Block&) const
 *
 * \ingroup group_traits
 */
template <class T>
using has_owns = decltype(std::declval<T&>().owns(std::declval<block>()));

template <typename T>
using has_owns_v = is_detected_exact<bool, has_owns, T>;

/**
 * This traits returns true if both passed types have the same type, resp.
 * template base type
 *
 * e.g. both_same_base<stack_allocator<32>, stack_allocator<64>>::value == true
 *
 * It's usage is not absolute safe, because it would mean to unroll all possible
 * parameter combinations.
 * But all currently available allocator should work.
 * \ingroup group_traits
 */

template <class T1, class T2>
struct both_same_base : std::false_type {};

template <class T1>
struct both_same_base<T1, T1> : std::true_type {};

template <template <size_t> class Allocator, size_t P1, size_t P2>
struct both_same_base<Allocator<P1>, Allocator<P2>> : std::true_type {};

template <template <size_t, size_t> class Allocator, size_t P1, size_t P2, size_t P3, size_t P4>
struct both_same_base<Allocator<P1, P2>, Allocator<P3, P4>> : std::true_type {};

template <template <size_t, size_t, size_t> class Allocator,
          size_t P1,
          size_t P2,
          size_t P3,
          size_t P4,
          size_t P5,
          size_t P6>
struct both_same_base<Allocator<P1, P2, P3>, Allocator<P4, P5, P6>> : std::true_type {};

template <template <size_t, size_t, size_t, size_t> class Allocator,
          size_t P1,
          size_t P2,
          size_t P3,
          size_t P4,
          size_t P5,
          size_t P6,
          size_t P7,
          size_t P8>
struct both_same_base<Allocator<P1, P2, P3, P4>, Allocator<P5, P6, P7, P8>> : std::true_type {};

template <template <class> class Allocator, class A1, class A2>
struct both_same_base<Allocator<A1>, Allocator<A2>> : std::true_type {};

template <template <class, size_t> class Allocator, class A1, size_t P1, class A2, size_t P2>
struct both_same_base<Allocator<A1, P1>, Allocator<A2, P2>> : std::true_type {};

template <template <class, size_t, size_t> class Allocator,
          class A1,
          size_t P1,
          size_t P2,
          class A2,
          size_t P3,
          size_t P4>
struct both_same_base<Allocator<A1, P1, P2>, Allocator<A2, P3, P4>> : std::true_type {};

template <template <class, size_t, size_t, size_t> class Allocator,
          class A1,
          size_t P1,
          size_t P2,
          size_t P3,
          class A2,
          size_t P4,
          size_t P5,
          size_t P6>
struct both_same_base<Allocator<A1, P1, P2, P3>, Allocator<A2, P4, P5, P6>> : std::true_type {};

template <template <class, size_t, size_t, size_t, size_t> class Allocator,
          class A1,
          size_t P1,
          size_t P2,
          size_t P3,
          size_t P4,
          class A2,
          size_t P5,
          size_t P6,
          size_t P7,
          size_t P8>
struct both_same_base<Allocator<A1, P1, P2, P3, P4>, Allocator<A2, P5, P6, P7, P8>>
    : std::true_type {};

/**
 * This class implements or hides, depending on the Allocators properties, the
 * expand operation.
 *
 * \ingroup group_traits
 */
template <class Allocator, typename Enabled = void>
struct expander;

template <class Allocator>
struct expander<Allocator, typename std::enable_if_t<has_expand_v<Allocator>>> {
    static bool apply(Allocator& a, block& b, size_t delta) noexcept { return a.expand(b, delta); }
};

template <class Allocator>
struct expander<Allocator, typename std::enable_if_t<!has_expand_v<Allocator>>> {
    static bool apply(Allocator&, block&, size_t) noexcept { return false; }
};

/**
 * This class implements or hides, depending on the Allocators properties, the
 * deallocate_all operation.
 *
 * \ingroup group_traits
 */
template <class Allocator, typename Enabled = void>
struct all_deallocator;

template <class Allocator>
struct all_deallocator<Allocator, typename std::enable_if_t<has_deallocate_all_v<Allocator>>> {
    static void apply(Allocator& a) noexcept { a.deallocate_all(); }
};

template <class Allocator>
struct all_deallocator<Allocator, typename std::enable_if<!has_deallocate_all_v<Allocator>>> {
    static void apply(Allocator&) noexcept {}
};

/**
 * traits that defines "type" A or B depending on the passed bool
 * \tparam A This type is defined if the bool is true
 * \tparam B This type is defined if the bool is false
 * \tparam bool Selects between the passed template parameter A or B
 *
 * \ingroup group_traits
 */
template <class A, class B, bool>
struct type_switch;

template <class A, class B>
struct type_switch<A, B, true> {
    using type = A;
};

template <class A, class B>
struct type_switch<A, B, false> {
    using type = B;
};
} // namespace traits
} // namespace v_100

using namespace v_100;
} // namespace alb

#endif