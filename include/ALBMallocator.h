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
  class Mallocator {

  public:
    Block allocate(size_t n) {
      return Block(::malloc(n), n);
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<Mallocator>::isHandledDefault(*this, b, n)) {
        return true;
      }

      Block reallocatedBlock(::realloc(b.ptr, n), n);
      
      if (reallocatedBlock.ptr != nullptr) {
        b = reallocatedBlock;
        return true;
      }
      return false;
    }

    void deallocate(Block& b) {
      if (b) {
        ::free(b.ptr);
        b.reset();
      }
    }
  };
}
