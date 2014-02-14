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

#include <boost/lockfree/stack.hpp>
#include <stdio.h>
#include "ALBAllocatorBase.h"

namespace ALB
{
  /**
   * Generic memory management of memory allocated on the stack.
   * This class is (and cannot be) thread safe!
   */
  template <size_t MaxSize, size_t DefaultAlignment = 16>
  class StackAllocator {
    static const size_t max_size = MaxSize;
    static const size_t alignment = DefaultAlignment;

    char _data[MaxSize];
    char* _p;

    bool isLastUsedBlock(const Block& block) {
      return (static_cast<char*>(block.ptr) + block.length == _p);
    }

  public:
    StackAllocator() : _p(_data) {}

    ~StackAllocator() {}

    Block allocate(size_t n) {
      Block result;

      if (n == 0) {
        return result;
      }

      const auto alignedLength = Helper::roundToAlignment<DefaultAlignment>(n);
      if (alignedLength + _p > _data + MaxSize) { // not enough memory left
        return result;
      }

      result.ptr = _p;
      result.length = n;

      _p += alignedLength;
      return result;
    }

    void deallocate(Block& b) {
      // If it was the most recent allocated MemoryBlock, then we can re-use the memory. Otherwise
      // this freed MemoryBlock is not available for further allocations. Since all happens on the stack
      // this is not a leak!
      if (isLastUsedBlock(b)) {
        _p = static_cast<char*>(b.ptr);
      }
      b.reset();
    }

    bool reallocate(Block& b, size_t n) {
      if (b.length == n) {
        return true;
      }

      if (n == 0) {
        deallocate(b);
        return true;
      }

      if (!b) {
        b = allocate(n);
        return true;
      }

      const auto alignedLength = Helper::roundToAlignment<DefaultAlignment>(n);

      if ( isLastUsedBlock(b) ) {
        if (static_cast<char*>(b.ptr) + alignedLength < _data + MaxSize) {
          b.length = alignedLength;
          _p = static_cast<char*>(b.ptr) + alignedLength;
          return true;
        }
        else { // out of memory
          return false;
        }
      }
      else {
        if (b.length > n) {
          b.length = Helper::roundToAlignment<DefaultAlignment>(n);
          return true;
        }
      }

      
      auto newBlock = allocate(alignedLength); // we cannot deallocate the old block, because it is in between used ones.
      if (newBlock) {
        Helper::blockCopy(b, newBlock);
        b = newBlock;
        return true;
      }
      return false;
    }

    bool expand(Block& b, size_t delta) {
      if (delta == 0) {
        return true;
      }
      if (!b) {
        b = allocate(delta);
        return b.length != 0;
      }
      if (!isLastUsedBlock(b)) {
        return false;
      }
      if (_p + delta > _data + MaxSize) {
        return false;
      }
      _p += delta;
      b.length += delta;
      return true;
    }

    bool owns(const Block& b) const {
      return (b.ptr >= _data && b.ptr < _data + MaxSize);
    }

    void deallocateAll() {
      _p = _data;
    }
  };
}
