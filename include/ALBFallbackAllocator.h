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

    typename Traits::enable_result_to<bool,
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


    typename Traits::enable_result_to<bool, 
      Traits::has_owns<Primary>::value && Traits::has_owns<Fallback>::value
    >::type owns(const Block& b) {
      return Primary::owns(b) || Fallback::owns(b);
    }

    typename Traits::enable_result_to<void,
      Traits::has_deallocateAll<Primary>::value && Traits::has_deallocateAll<Fallback>::value
    >::type  deallocateAll() {
      Primary::deallocateAll();
      Fallback::deallocateAll();
    }
  };
}
