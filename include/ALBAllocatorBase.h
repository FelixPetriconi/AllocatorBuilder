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
#include "ALBTraits.h"

namespace ALB
{
  /**
   * Value type to describe a memory block and it's length
   */
  struct Block {
    Block() : ptr(nullptr), length(0) {}
    
    Block(void* ptr, size_t length) 
      : ptr(ptr)
      , length(length) 
    {}

    /**
     * During destruction of any of this instance, the described memory
     * is not freed!
     */
    ~Block() {}

    /**
     * Clears the block
     */
    void reset() {
      ptr = nullptr;
      length = 0;
    }

    /**
     * Bool operator to make the Allocator code better readable
     */
    operator bool() const { 
      return length != 0; 
    }

    /// This points to the start address of the described memory block
    void* ptr;

    /// This describes the length of the reserved bytes.
    size_t length;
  };

  /**
   * Flag to be used inside the Dynastic struct to signal that the value
   * can be changed during runtime.
   */
  static const int DynamicSetSize = -2;



  namespace Helper {
    /**
     * Copies std::min(source.length, destination.length) bytes from source to destination
     */
    void blockCopy(const ALB::Block& source, ALB::Block& destination);


    /**
     * Returns a upper rounded value of multiples of a
     */
    inline size_t roundToAlignment(size_t basis, size_t n) {
      auto remainder = n % basis;
      return n + ((remainder == 0)? 0 : (basis - remainder));
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
     * The Reallocator handles standard use cases during the deallocation.
     * If available it uses ::expand() of the allocator.
     * (With C++11 this could be done with a partial specialized function,
     * but VS 2012 does not support this.)
     */
    template <class Allocator, typename Enabled = void>
    struct Reallocator;

    /**
     * Specialization for Allocators that implements ::expand()
     */
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

    /**
     * Specialization for Allocators, that don't implement ::expand()
     */
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
     * @Author Andrei Alexandrescu
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
      Dynastic() : _v(-1) {}
      size_t value() const { return _v; }
      void value(size_t w) { _v = w; }
    };
  } // namespace Helper
}
