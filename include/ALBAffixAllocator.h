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
#include <boost/assert.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4345)
#endif

namespace ALB
{
  namespace AffixAllocatorHelper {
    /**
     * This special kind of NoAffix is necessary to have the possibility to test the
     * AffixAllocator in a generic way. This must be used to disable a Prefix or
     * Sufix.
     */
    struct NoAffix {
      typedef size_t value_type;
      static const size_t pattern = 0;
    };
  }

  /**
   * This allocator enables the possibility to surround allocated memory blocks with
   * guards, ref-counter, mutex or etc.
   * It automatically places an object of type Prefix before the returned memory location 
   * and an object of type Sufix after it. In case that they are of type AffixAllocatorHelper::Empty 
   * nothing is inserted.
   * Prefix and Sufix, if used, must be trivially copyable. (This cannot be statically asserted,
   * because this would block the possibility to use this allocator as guard for memory
   * under- or overflow.
   */
  template <class Allocator, typename Prefix, typename Sufix = AffixAllocatorHelper::NoAffix>
  class AffixAllocator {
    Allocator _allocator;
    
    Prefix* innerToPrefix(const Block& b) const {
      return reinterpret_cast<Prefix*>(static_cast<Prefix*>(b.ptr));
    }

    Sufix* innerToSufix(const Block& b) const {
      return reinterpret_cast<Sufix*>(static_cast<char*>(b.ptr) + b.length - sufix_size);
    }

    Prefix* outerToPrefix(const Block& b) const {
      return reinterpret_cast<Prefix*>(static_cast<Prefix*>(b.ptr) - 1);
    }

    Sufix* outerToSufix(const Block& b) const {
      return reinterpret_cast<Sufix*>(static_cast<char*>(b.ptr) + b.length);
    }

    Block toInnerBlock(const Block& b) const {
      return Block(static_cast<char*>(b.ptr) - prefix_size, b.length + prefix_size + sufix_size);
    }

    Block toOuterBlock(const Block& b) const {
      return Block(static_cast<char*>(b.ptr) + prefix_size, b.length - prefix_size - sufix_size);
    }

  public:
    typedef Allocator allocator;
    typename typedef Prefix prefix;
    typename typedef Sufix sufix;

    static const size_t prefix_size = std::is_same<Prefix, AffixAllocatorHelper::NoAffix>::value? 0 : sizeof(Prefix);
    static const size_t sufix_size = std::is_same<Sufix, AffixAllocatorHelper::NoAffix>::value? 0 : sizeof(Sufix);

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
        return toOuterBlock(innerMem);
      }
      return Block();
    }

    void deallocate(Block& b) {
      if (!b) {
        return;
      }    
      BOOST_ASSERT_MSG(owns(b), "It is not wise to let me deallocate a foreign Block!");
      if (!owns(b)) {
        return;
      }

      if (prefix_size > 0) {
        outerToPrefix(b)->~Prefix();
      }
      if (sufix_size > 0) {
        outerToSufix(b)->~Sufix();
      }
      _allocator.deallocate(toInnerBlock(b));
      b.reset();
    }

    bool owns(const Block& b) const {
      return b && _allocator.owns(toInnerBlock(b));
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<AffixAllocator>::isHandledDefault(*this, b, n)) {
        return true;
      }
      auto innerBlock = toInnerBlock(b);

      // Remember the old Sufix in case that it is available, because it must
      // be later placed to the new position
      std::unique_ptr<Sufix> oldSufix(sufix_size > 0? new Sufix(*outerToSufix(b)) : nullptr);

      if (_allocator.reallocate(innerBlock, n + prefix_size + sufix_size)) {
        if (sufix_size > 0) {
          new (innerToSufix(innerBlock)) Sufix(*oldSufix);
        }
        b = toOuterBlock(innerBlock);
        return true;
      }
      return false;
    }

    // Make the function invisible for the has_expand<> trait if the dependent type
    // Allocator does not implement expand
    typename Traits::enabled<Traits::has_expand<Allocator>::value>::type 
      expand(Block& b, size_t delta) 
    {
      if (delta == 0) {
        return true;
      }

      if (!b) {
        b = allocate(delta);
        return b;
      }

      auto oldBlock = b;
      auto innerBlock = toInnerBlock(b);
      
      if (_allocator.expand(innerBlock, delta)) {
        if (sufix_size > 0) {
          new (innerToSufix(innerBlock)) Sufix(*outerToSufix(oldBlock));
        }
        b = toOuterBlock(innerBlock);
        return true;
      }
      return false;
    }

  };
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

