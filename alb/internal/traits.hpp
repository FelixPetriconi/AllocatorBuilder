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

namespace alb {
  struct block;

  namespace traits {
    /**
     * Trait that checks if the given class implements bool expand(Block&, size_t)
     *
     * \ingroup group_traits
     */
    template <typename T> struct has_expand {
    private:
      typedef char Yes;
      struct No {
        char dummy[2];
      };

      template <typename U, bool (U::*)(block &, size_t)> struct Check;
      template <typename U> static Yes func(Check<U, &U::expand> *);
      template <typename U> static No func(...);

    public:
      static const bool value = (sizeof(func<T>(nullptr)) == sizeof(Yes));
    };

    /**
     * Trait that checks if the given class implements void deallocateAll()
     *
     * \ingroup group_traits
     */
    template <typename T> struct has_deallocateAll {
    private:
      typedef char Yes;
      struct No {
        char dummy[2];
      };

      template <typename U, void (U::*)()> struct Check;
      template <typename U> static Yes func(Check<U, &U::deallocateAll> *);
      template <typename U> static No func(...);

    public:
      static const bool value = (sizeof(func<T>(nullptr)) == sizeof(Yes));
    };

    /**
     * Trait that checks if the given class implements bool owns(const Block&) const
     *
     * \ingroup group_traits
     */
    template <typename T> struct has_owns {
    private:
      typedef char Yes;
      struct No {
        char dummy[2];
      };

      template <typename U, bool (U::*)(const block &) const> struct Check;
      template <typename U> static Yes func(Check<U, &U::owns> *);
      template <typename U> static No func(...);

    public:
      static const bool value = (sizeof(func<T>(nullptr)) == sizeof(Yes));
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
      static bool doIt(Allocator &a, block &b, size_t delta)
      {
        return a.expand(b, delta);
      }
    };

    template <class Allocator>
    struct Expander<Allocator, typename std::enable_if<!has_expand<Allocator>::value>::type> {
      static bool doIt(Allocator &, block &, size_t)
      {
        return false;
      }
    };

    /**
    * This class implements or hides, depending on the Allocators properties, the
    * deallocateAll operation.
    *
    * \ingroup group_traits
    */
    template <class Allocator, typename Enabled = void> struct AllDeallocator;

    template <class Allocator>
    struct AllDeallocator<Allocator,
                          typename std::enable_if<has_deallocateAll<Allocator>::value>::type> {
      static void doIt(Allocator &a)
      {
        a.deallocateAll();
      }
    };

    template <class Allocator>
    struct AllDeallocator<Allocator,
                          typename std::enable_if<!has_deallocateAll<Allocator>::value>::type> {
      static void doIt(Allocator &)
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
