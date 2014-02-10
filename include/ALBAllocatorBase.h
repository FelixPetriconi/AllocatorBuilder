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

  /**
   * Flag to be used inside the Dynastic struct
   */
  static const int DynamicSetSize = -2;


  /**
   * Value type to hold a memory block and it's length
   */
  struct Block {
    Block() : ptr(nullptr), length(0) {}
    
    Block(void* ptr, size_t length) 
      : ptr(ptr)
      , length(length) 
    {}

    ~Block() {}

    void reset() {
      ptr = nullptr;
      length = 0;
    }

    operator bool() const { 
      return length != 0; 
    }

    void* ptr;
    size_t length;
  };

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
  }

  namespace Helper {
    /**
     * Copies std::min(source.length, destination.length) bytes from source to destination
     */
    void blockCopy(const ALB::Block& source, ALB::Block& destination);


    /**
     * Returns a upper rounded value of multiples of a
     */
    template <size_t a>
    inline size_t roundToAlignment(size_t n) {
      static_assert(a % 4 == 0, "Alignment must be a multiple of 4");

      auto remainder = n % a;
      return n + ((remainder == 0)? 0 : (a - remainder));
    }

    /**
     * Allocates a new block of n bytes with newAllocator, copies min(b.length, n) bytes to it,
     * deallocates the old block b, and returns the new block.
     */
    template <class OldAllocator, class NewAllocator>
    bool reallocateWithCopy(OldAllocator& oldAllocator, NewAllocator& newAllocator, Block& b, size_t n)
    {
      auto newBlock = newAllocator.allocate(n);
      if (!newBlock) {
        return false;
      }
      
      blockCopy(b, newBlock);
      oldAllocator.deallocate(b);
      b = newBlock;

      return true;
    }

    /**
     * The Reallocator handles standard use cases during the deallocation
     * If available it uses ::expand() of the allocator
     */
    template <class Allocator, typename Enabled = void>
    struct Reallocator;

    template <class Allocator>
    struct Reallocator<Allocator, typename std::enable_if<Traits::has_expand<Allocator>::value>::type>
    {
      static bool isHandledDefault(Allocator& allocator, Block& b, size_t n)
      {
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

    template <class Allocator>
    struct Reallocator<Allocator, typename std::enable_if<!Traits::has_expand<Allocator>::value>::type>
    {
      static bool isHandledDefault(Allocator& allocator, Block& b, size_t n)
      {
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

    /**
     * Simple generic value type that is either compile time constant or dynamically set-able
     * depending of DynamicEnableSwitch. If v and DynamicEnableSwitch, then value can be changed
     * during runtime.
     */
    template <size_t v, size_t DynamicEnableSwitch>
    struct Dynastic {
      size_t value() const { return v; }
    };

    template <size_t DynamicEnableSwitch>
    struct Dynastic <DynamicEnableSwitch, DynamicEnableSwitch> {
    private:
      size_t _v;
    public:
      Dynastic() : _v(0) {}
      size_t value() const { return _v; }
      void value(size_t w) { _v = w; }
    };
  } // namespace Helper
}
