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
  template <size_t DefaultAlignment = 16>
  class Mallocator {
    static const size_t alignment = DefaultAlignment;

  public:
    Block allocate(size_t n) {
      return Block(_aligned_malloc(n, DefaultAlignment), n);
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<Mallocator>::isHandledDefault(*this, b, n)) {
        return true;
      }

      Block reallocatedBlock(_aligned_realloc(b.ptr, n, DefaultAlignment), n);
      
      if (reallocatedBlock.ptr != nullptr) {
        b = reallocatedBlock;
        return true;
      }
      return false;
    }

    void deallocate(Block& b) {
      if (b) {
        _aligned_free(b.ptr);
        b.reset();
      }
    }
  };
}
