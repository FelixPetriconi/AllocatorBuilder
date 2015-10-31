///////////////////////////////////////////////////////////////////
//
// Copyright 2015 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////

#pragma once
#include "allocator_base.hpp"

#include <cassert>

namespace alb
{
  inline namespace v_100
  {
    class null_allocator
    {
    public:
      static const unsigned alignment = 64 * 1024;
      
      block allocate(size_t) noexcept { 
        return {nullptr, 0}; 
      }

      bool owns(const block& b) noexcept { 
        return !b; 
      }

      bool expand(block& b, size_t) noexcept {
        assert(!b); 
        return false;
      }

      bool reallocate(block& b, size_t) noexcept {
        assert(!b); 
        return false;
      }

      void deallocate(block& b) noexcept { 
        assert(!b); 
      }

      void deallocateAll() noexcept { 
      }
    };
  }
  using namespace v_100;
}
