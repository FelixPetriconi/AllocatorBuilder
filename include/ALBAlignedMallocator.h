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
  class AlignedMallocator {
    static const size_t alignment = DefaultAlignment;

#ifdef _MSC_VER
    bool alignedReallocate(Block& b, size_t n) {
      Block reallocatedBlock(_aligned_realloc(b.ptr, n, DefaultAlignment), n);
      
      if (reallocatedBlock.ptr != nullptr) {
        b = reallocatedBlock;
        return true;
      }
      return false;
    }
#else
    // On posix there is no _aligned_realloc so we try a normal realloc
    // if the result is still aligned we are fine
    // otherwise we have to do it by hand
    bool alignedReallocate(Block& b, size_t n) {
      Block reallocatedBlock(::realloc(b.ptr, n));
      if (reallocatedBlock.ptr != nullptr) {
        if (static_cast<size_t>(b.ptr) % DefaultAlignment != 0) {
          auto newAlignedBlock = allocate(n);
          if (!newAlignedBlock) {
            return false;
          }
          Helper::blockCopy(b, newAlignedBlock);
        }
        else {
          b = reallocatedBlock;  
        }
      
        return true;
      }
      return false;
    }
#endif

  public:
    Block allocate(size_t n) {
#ifdef _MSC_VER      
      return Block(_aligned_malloc(n, DefaultAlignment), n);
#else
      return Block(void *memalign(DefaultAlignment, n), n);
#endif      
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<Mallocator>::isHandledDefault(*this, b, n)) {
        return true;
      }

      return alignedReallocate(b, n);
    }

    void deallocate(Block& b) {
      if (b) {
#ifdef _MSC_VER        
        _aligned_free(b.ptr);
#else
        ::free(b.ptr);
#endif        
        b.reset();
      }
    }
  };
}
