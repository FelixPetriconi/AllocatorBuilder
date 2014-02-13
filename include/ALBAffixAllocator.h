///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
//////////////////////////////////////////////////////////////////
#pragma once

#include "ALBAllocatorBase.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4345)
#endif

namespace ALB
{
  struct Empty {};

  template <class Allocator, typename Prefix, typename Sufix = Empty>
  class AffixAllocator {
    typedef Allocator allocator;
    static const size_t prefix_size = sizeof(Prefix);
    static const size_t sufix_size = sizeof(Sufix);
    
    Allocator _allocator;
    
    Prefix* innerToPrefix(const Block& b) {
      return reinterpret_cast<Prefix*>(static_cast<Prefix*>(b.ptr));
    }

    Sufix* innerToSufix(const Block& b) {
      return reinterpret_cast<Sufix*>(static_cast<char*>(b.ptr) + b.length - sufix_size);
    }

    Prefix* outerToPrefix(const Block& b) {
      return reinterpret_cast<Prefix*>(static_cast<Prefix*>(b.ptr) - 1);
    }

    Sufix* outerToSufix(const Block& b) {
      return reinterpret_cast<Sufix*>(static_cast<char*>(b.ptr) + b.length);
    }

  public:
    Block allocate(size_t n) {
      if (n == 0) {
        return Block();
      }

      auto innerMem = _allocator.allocate(prefix_size + n + sufix_size);
      if (innerMem) {
        if (prefix_size > 0) {
          new (innerToPrefix(innerMem)) Prefix();
        }
        if (sufix_size > 0) {
          new (innerToSufix(innerMem)) Sufix();
        }
        return Block(static_cast<char*>(innerMem.ptr) + prefix_size, 
                      innerMem.length - prefix_size - sufix_size);
      }
      return Block();
    }

    void deallocate(Block& b) {
      if (!b) {
        return;
      }

      if (prefix_size > 0) {
        outerToPrefix(b)->~Prefix();
      }
      if (sufix_size > 0) {
        outerToSufix(b)->~Sufix();
      }
      _allocator.deallocate(Block(static_cast<char*>(b.ptr) - prefix_size, b.length + prefix_size + sufix_size));
      b.reset();
    }
  };
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

