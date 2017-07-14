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

#include <type_traits>
#include <stddef.h>
#include "block.hpp"

namespace alb {

  inline namespace v_100 {

    namespace traits {
      /**
       * Trait that checks if the given class implements bool expand(Block&, size_t)
       *
       * \ingroup group_traits
       */
      template <typename T> struct has_expand 
      {
        template <typename U, bool (U::*)(block &, size_t) noexcept> struct Check;
        template <typename U> static constexpr bool test(Check<U, &U::expand> *) { return true; }
        template <typename U> static constexpr bool test(...) { return false; }

        static constexpr bool value = test<T>(nullptr);
      };

      /**
       * Trait that checks if the given class implements void deallocate_all()
       *
       * \ingroup group_traits
       */
      template <typename T> struct has_deallocate_all 
      {
        template <typename U, void (U::*)()noexcept> struct Check;
        template <typename U> static bool test(Check<U, &U::deallocate_all> *) { return true; }
        template <typename U> static bool test(...) { return false; }

        static constexpr bool value = test<T>(nullptr);
      };

      /**
       * Trait that checks if the given class implements bool owns(const Block&) const
       *
       * \ingroup group_traits
       */
      template <typename T> struct has_owns 
      {
        template <typename U, bool (U::*)(const block &) const noexcept> struct Check;
        template <typename U> static constexpr bool test(Check<U, &U::owns> *) { return true; }
        template <typename U> static constexpr bool test(...) { return false; }

        static constexpr bool value = test<T>(nullptr);
      };

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

      template <class T1, class T2> struct both_same_base : std::false_type {
      };

      template <class T1> struct both_same_base<T1, T1> : std::true_type {
      };

      template <template <size_t> class Allocator, size_t P1, size_t P2>
      struct both_same_base<Allocator<P1>, Allocator<P2>> : std::true_type {
      };

      template <template <size_t, size_t> class Allocator, size_t P1, size_t P2, size_t P3, size_t P4>
      struct both_same_base<Allocator<P1, P2>, Allocator<P3, P4>> : std::true_type {
      };

      template <template <size_t, size_t, size_t> class Allocator, size_t P1, size_t P2, size_t P3,
        size_t P4, size_t P5, size_t P6>
      struct both_same_base<Allocator<P1, P2, P3>, Allocator<P4, P5, P6>> : std::true_type {
      };

      template <template <size_t, size_t, size_t, size_t> class Allocator, size_t P1, size_t P2,
        size_t P3, size_t P4, size_t P5, size_t P6, size_t P7, size_t P8>
      struct both_same_base<Allocator<P1, P2, P3, P4>, Allocator<P5, P6, P7, P8>> : std::true_type {
      };

      template <template <class> class Allocator, class A1, class A2>
      struct both_same_base<Allocator<A1>, Allocator<A2>> : std::true_type {
      };

      template <template <class, size_t> class Allocator, class A1, size_t P1, class A2, size_t P2>
      struct both_same_base<Allocator<A1, P1>, Allocator<A2, P2>> : std::true_type {
      };

      template <template <class, size_t, size_t> class Allocator, class A1, size_t P1, size_t P2,
      class A2, size_t P3, size_t P4>
      struct both_same_base<Allocator<A1, P1, P2>, Allocator<A2, P3, P4>> : std::true_type {
      };

      template <template <class, size_t, size_t, size_t> class Allocator, class A1, size_t P1,
        size_t P2, size_t P3, class A2, size_t P4, size_t P5, size_t P6>
      struct both_same_base<Allocator<A1, P1, P2, P3>, Allocator<A2, P4, P5, P6>> : std::true_type {
      };

      template <template <class, size_t, size_t, size_t, size_t> class Allocator, class A1, size_t P1,
        size_t P2, size_t P3, size_t P4, class A2, size_t P5, size_t P6, size_t P7, size_t P8>
      struct both_same_base<Allocator<A1, P1, P2, P3, P4>, Allocator<A2, P5, P6, P7, P8>>
        : std::true_type {
      };

      /**
      * This class implements or hides, depending on the Allocators properties, the
      * expand operation.
      *
      * \ingroup group_traits
      */
      template <class Allocator, typename Enabled = void> struct Expander;

      template <class Allocator>
      struct Expander<Allocator, typename std::enable_if<has_expand<Allocator>::value>::type> {
        static bool do_it(Allocator &a, block &b, size_t delta) noexcept
        {
          return a.expand(b, delta);
        }
      };

      template <class Allocator>
      struct Expander<Allocator, typename std::enable_if<!has_expand<Allocator>::value>::type> {
        static bool do_it(Allocator &, block &, size_t) noexcept
        {
          return false;
        }
      };

      /**
      * This class implements or hides, depending on the Allocators properties, the
      * deallocate_all operation.
      *
      * \ingroup group_traits
      */
      template <class Allocator, typename Enabled = void> struct AllDeallocator;

      template <class Allocator>
      struct AllDeallocator<Allocator,
        typename std::enable_if<has_deallocate_all<Allocator>::value>::type> {
        static void do_it(Allocator &a) noexcept
        {
          a.deallocate_all();
        }
      };

      template <class Allocator>
      struct AllDeallocator<Allocator,
        typename std::enable_if<!has_deallocate_all<Allocator>::value>::type> {
        static void do_it(Allocator &) noexcept
        {
        }
      };

      /**
       * traits that defines "type" A or B depending on the passed bool
       * \tparam A This type is defined if the bool is true
       * \tparam B This type is defined if the bool is false
       * \tparam bool Selects between the passed template parameter A or B
       *
       * \ingroup group_traits
       */
      template <class A, class B, bool> struct type_switch;

      template <class A, class B> struct type_switch<A, B, true> {
        using type = A;
      };

      template <class A, class B> struct type_switch<A, B, false> {
        using type = B;
      };
    }
  }

  using namespace v_100;
}
