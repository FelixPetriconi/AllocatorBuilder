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

namespace ALB
{
  struct Block;

  namespace Traits
  {
    /**
     * Trait that checks if the given class implements bool expand(Block&, size_t)
     */
    template<typename T>
    struct has_expand
    {
    private:
      typedef char   Yes;
      typedef struct No { char dummy[2]; };

      template <typename U, bool (U::*)(Block&, size_t)> struct Check;
      template <typename U> static Yes func(Check<U, &U::expand> *);
      template <typename U> static No func(...);
    public:
      static const bool value = (sizeof(func<T>(0)) == sizeof(Yes));
    };

    /**
     * Trait that checks if the given class implements void deallocateAll()
     */
    template<typename T>
    struct has_deallocateAll
    {
    private:
      typedef char   Yes;
      typedef struct No { char dummy[2]; };

      template <typename U, void (U::*)()> struct Check;
      template <typename U> static Yes func(Check<U, &U::deallocateAll> *);
      template <typename U> static No func(...);
    public:
      static const bool value = (sizeof(func<T>(0)) == sizeof(Yes));
    };

    /**
     * Trait that checks if the given class implements bool owns(const Block&) const
     */
    template<typename T>
    struct has_owns
    {
    private:
      typedef char   Yes;
      typedef struct No { char dummy[2]; };

      template <typename U, bool (U::*)(const Block&) const> struct Check;
      template <typename U> static Yes func(Check<U, &U::owns> *);
      template <typename U> static No func(...);
    public:
      static const bool value = (sizeof(func<T>(0)) == sizeof(Yes));
    };


    /**
     * The following traits define a type bool, if the given Allocators implement expand,
     * otherwise it hides the signature bool expand(Block&, size_t) for has_expand<>
     */

    struct Disabled;

    template <class A, class B = A, typename Enabled = void>
    struct expand_enabled;

    template <class A, class B>
    struct expand_enabled<A, B, 
      typename std::enable_if<has_expand<A>::value && has_expand<B>::value>::type>
    {
      typedef bool type;
    };

    template <class A, class B>
    struct expand_enabled<A, B, 
      typename std::enable_if<!has_expand<A>::value || !has_deallocateAll<B>::value>::type>
    {
      typedef Disabled type;
    };


    /**
     * The following traits define a type void, if the given Allocators implement deallocateAll,
     * otherwise it hides the signature void deallocateAll() for has_deallocateAll<>
     */
    template <class A, class B = A, typename Enabled = void>
    struct deallocateAll_enabled;

    template <class A, class B>
    struct deallocateAll_enabled<A, B, 
      typename std::enable_if<has_deallocateAll<A>::value && has_deallocateAll<B>::value>::type>
    {
      typedef bool type;
    };

    template <class A, class B>
    struct deallocateAll_enabled<A, B, 
      typename std::enable_if<!has_deallocateAll<A>::value || !has_deallocateAll<B>::value>::type>
    {
      typedef Disabled type;
    };

    /**
     * This traits returns true if both passed types have the same type, resp. template base type
     * 
     * e.g. both_same_base<StackAllocator<32>, StackAllocator<64>>::value == true
     * 
     * It's usage is not absolute safe, because it would mean to unroll all possible parameter combinations.
     * But all currently available allocator should work.
     */
    template<class T1, class T2>
    struct both_same_base : std::false_type
    {};

    template<class T1>
    struct both_same_base<T1,T1> : std::true_type
    {};

    template<template <size_t> class Allocator, size_t P1, size_t P2>
    struct both_same_base<Allocator<P1>, Allocator<P2>> : std::true_type
    {};

    template<template <size_t, size_t> class Allocator, size_t P1, size_t P2, size_t P3, size_t P4>
    struct both_same_base<Allocator<P1,P2>, Allocator<P3, P4>> : std::true_type
    {};

    template<template <size_t, size_t, size_t> class Allocator, size_t P1, size_t P2, size_t P3, size_t P4, size_t P5, size_t P6>
    struct both_same_base<Allocator<P1,P2,P3>, Allocator<P4, P5, P6>> : std::true_type
    {};

    template<template <size_t, size_t, size_t, size_t> class Allocator, size_t P1, size_t P2, size_t P3, size_t P4, size_t P5, size_t P6, size_t P7, size_t P8>
    struct both_same_base<Allocator<P1,P2,P3, P4>, Allocator<P5, P6, P7, P8>> : std::true_type
    {};

    template<template <class> class Allocator, class A1, class A2>
    struct both_same_base<Allocator<A1>, Allocator<A2>> : std::true_type
    {};

    template<template <class, size_t> class Allocator, class A1, size_t P1, class A2, size_t P2>
    struct both_same_base<Allocator<A1, P1>, Allocator<A2, P2>> : std::true_type
    {};

    template<template <class, size_t, size_t> class Allocator, class A1, size_t P1, size_t P2, class A2, size_t P3, size_t P4>
    struct both_same_base<Allocator<A1, P1,P2>, Allocator<A2, P3, P4>> : std::true_type
    {};

    template<template <class, size_t, size_t, size_t> class Allocator, class A1, size_t P1, size_t P2, size_t P3, class A2, size_t P4, size_t P5, size_t P6>
    struct both_same_base<Allocator<A1, P1,P2,P3>, Allocator<A2, P4, P5, P6>> : std::true_type
    {};

    template<template <class, size_t, size_t, size_t, size_t> class Allocator, class A1, size_t P1, size_t P2, size_t P3, size_t P4, class A2, size_t P5, size_t P6, size_t P7, size_t P8>
    struct both_same_base<Allocator<A1, P1,P2,P3, P4>, Allocator<A2, P5, P6, P7, P8>> : std::true_type
    {};



  }
}
