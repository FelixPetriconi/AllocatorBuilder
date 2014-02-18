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

#include "ALBAllocatorBase.h"

namespace ALB
{
  template <class Primary, class Fallback>
  class FallbackAllocator : public Primary, public Fallback {
    typedef Primary primary_allocator;
    typedef Fallback fallback_allocator;

    static_assert( !Traits::both_same_base<Primary, Fallback>::value, "Primary- and Fallback-Allocator cannot be both of base!");

  public:
    Block allocate(size_t n) {
      Block result( Primary::allocate(n) );
      if (!result)
        result = Fallback::allocate(n);

      return result;
    }

    void deallocate(Block b) {
      if (Primary::owns(b))
        Primary::deallocate(b);
      else 
        Fallback::deallocate(b);
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator::isHandledDefault(*this, b, n)){
        return true;
      }

      if (Primary::owns(b)) {
        if (Primary::reallocate(b, n)) {
          return true;
        }
        return reallocateWithCopy(static_cast<Primary&>(*this), static_cast<Fallback&>(*this), b, n);
      }

      return Fallback::reallocate(b, n);
    }

    bool expand(Block& b, size_t delta) {
      static_assert(Traits::has_expand<Primary>::value, "Primary allocator does not implement expand()");
      static_assert(Traits::has_expand<Fallback>::value, "Fallback allocator does not implement expand()");

      if (Primary::owns(b)) {
        return Primary::expand(b, delta);
      }
      return Fallback::expand(b, delta);
    }


    bool owns(const Block& b) {
      return Primary::owns(b) || Fallback::owns(b);
    }

    void deallocateAll() {
      static_assert(Traits::has_deallocateAll<SmallAllocator>::value, "The SmallAllocator does not implement deallocateAll")
      static_assert(Traits::has_deallocateAll<Fallback>::value, "The Fallback does not implement deallocateAll")
      
      SmallAllocator::deallocateAll();
      Fallback::deallocateAll();
    }
  };
}
