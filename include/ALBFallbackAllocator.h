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
      if (n == 0) {
        return Block();
      }
      Block result( Primary::allocate(n) );
      if (!result)
        result = Fallback::allocate(n);

      return result;
    }

    void deallocate(Block& b) {
      if (!b) {
        return;
      }

      if (Primary::owns(b))
        Primary::deallocate(b);
      else 
        Fallback::deallocate(b);
    }

    bool reallocate(Block& b, size_t n) {
      if (Primary::owns(b)) {
        if (Helper::Reallocator<Primary>::isHandledDefault(static_cast<Primary&>(*this), b, n)) {
          return true;
        }
      } 
      else {
        if (Helper::Reallocator<Fallback>::isHandledDefault(static_cast<Fallback&>(*this), b, n)) {
          return true;
        }
      }

      if (Primary::owns(b)) {
        if (Primary::reallocate(b, n)) {
          return true;
        }
        return Helper::reallocateWithCopy(static_cast<Primary&>(*this), static_cast<Fallback&>(*this), b, n);
      }

      return Fallback::reallocate(b, n);
    }

    typename Traits::enabled<
      Traits::has_expand<Primary>::value || Traits::has_expand<Fallback>::value
    >::type expand(Block& b, size_t delta) 
    {
      if (Primary::owns(b)) {
        if (Traits::has_expand<Primary>::value) {
          return Traits::Expander<Primary>::doIt(static_cast<Primary&>(*this), b, delta);
        }
        else {
          return false;
        }
      }
      if (Traits::has_expand<Fallback>::value) {
        return Traits::Expander<Fallback>::doIt(static_cast<Fallback&>(*this), b, delta);
      }
      else {
        return false;
      }

      return false;
    }


    typename Traits::enabled<Traits::has_owns<Primary>::value && Traits::has_owns<Fallback>::value
    >::type owns(const Block& b) {
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
