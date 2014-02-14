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

    // forward declarations of all allocator
    template <size_t MaxSize, size_t Alignment=4>
    class StackAllocator;

    template<class Allocator>
    struct is_stackallocator : std::false_type
    {};

    template <size_t MaxSize>
    struct is_stackallocator<StackAllocator<MaxSize>> : std::true_type
    {};

    template <size_t MaxSize, size_t Alignment>
    struct is_stackallocator<StackAllocator<MaxSize, Alignment>> : std::true_type
    {};
  }
}
