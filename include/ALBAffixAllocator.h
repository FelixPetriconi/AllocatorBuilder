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
   * One should keep in mind, that using a Sufix is not CPU cache friendly!
   * @tparam Allocator The allocator that is used as underlying allocator
   * @tparam Prefix If defined, then an object of that kind is constructed in front of
   *                any returned block
   * @tparam Sufix If defined, then an object of that kind is constructed beyond any returned block
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

    /**
     * Allocates a @see Block of n bytes. Actually a Block of n + sizeof(Prefix) + sizeof(Sufix)
     * bytes is allocated. Depending of the defines Prefix and Sufix types objects of this
     * gets instantiated before and/or beyond the returned Block. 
     * If Zero bytes are allocated then no allocation at all takes places and an empty Block
     * is returned.
     * @param n Specifies the number of requested bytes. n or more bytes are returned, depending on
     *          the alignment of the underlying Allocator.
     */
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

    /**
     * The given block gets deallocated. If Prefix or Sufix are defined then their d'tor(s) are 
     * called.
     * @param b The @see Block that should be freed. An assertion is raised, if the block
     *          is not owned by the underlying Allocator
     */
    void deallocate(Block& b) {
      if (!b) {
        return;
      }    
      if (!owns(b)) {
        BOOST_ASSERT_MSG(false, "It is not wise to let me deallocate a foreign Block!");
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

    /**
     * If the underlying Allocator defines ::owns() this method is available.
     * It returns true, if the given block is owned by this allocator.
     * @param b The @see Block that should be checked for ownership
     */
    typename Traits::enable_result_to<bool, Traits::has_owns<Allocator>::value>::type owns(const Block& b) const {
      return b && _allocator.owns(toInnerBlock(b));
    }

    /**
     * The given block gets reallocated to the new provided size n. Any potential defined
     * Prefix and/or Sufix gets copied to the new location. 
     * @param b The block that should be resized
     * @param n The new size (n = zero means a deallocation)
     * @return True if the operation was successful
     */
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

    /**
     * The method tries to expand the given block by at least delta bytes insito at the 
     * given location. This only This is only available if the underlaying Allocator 
     * implemnts ::expand().
     * @param b The block that should be expanded
     * @param delta The number of bytes that the given block should be increased
     * @return True, if the operation was successful.
     */
    typename Traits::enable_result_to<bool, Traits::has_expand<Allocator>::value>::type 
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

