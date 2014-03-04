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
#include <memory>
#include <boost/assert.hpp>
#include <boost/optional.hpp>
#include <boost/config.hpp>
#include <boost/config/suffix.hpp>

#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
#include <boost/noncopyable.hpp>
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4345)
#endif

namespace ALB {
namespace AffixAllocatorHelper {
/**
 * This special kind of NoAffix is necessary to have the possibility to test the
 * AffixAllocator in a generic way. This must be used to disable a Prefix or
 * Sufix.
 */
struct NoAffix {
  typedef int value_type;
  BOOST_STATIC_CONSTANT(int, pattern = 0);
};
}

/**
 * This allocator enables the possibility to surround allocated memory blocks
 * with guards, ref-counter, mutex or etc.
 * It is used by the ALB::AllocatorWithStats
 * It automatically places an object of type Prefix before the returned memory
 * location and an object of type Sufix after it. In case that they are of type
 * AffixAllocatorHelper::Empty nothing is inserted.
 * Prefix and Sufix, if used, must be trivially copyable. (This cannot be
 * statically asserted, because this would block the possibility to use this
 * allocator as guard for memory under- or overflow.
 * One should keep in mind, that using a Sufix is not CPU cache friendly!
 * \tparam Allocator The allocator that is used as underlying allocator
 * \tparam Prefix If defined, then an object of that kind is constructed in
 *                front of any returned block
 * \tparam Sufix If defined, then an object of that kind is constructed beyond
 *               any returned block
 *
 * \ingroup group_allocators group_shared
 */
template <class Allocator, typename Prefix,
          typename Sufix = AffixAllocatorHelper::NoAffix>
class AffixAllocator
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
    : public boost::noncopyable
#endif
      {
  Allocator _allocator;

  Prefix *innerToPrefix(const Block &b) const {
    return reinterpret_cast<Prefix *>(static_cast<Prefix *>(b.ptr));
  }

  Sufix *innerToSufix(const Block &b) const {
    return reinterpret_cast<Sufix *>(static_cast<char *>(b.ptr) + b.length -
                                     sufix_size);
  }

  Block toInnerBlock(const Block &b) const {
    return Block(static_cast<char *>(b.ptr) - prefix_size,
                 b.length + prefix_size + sufix_size);
  }

  Block toOuterBlock(const Block &b) const {
    return Block(static_cast<char *>(b.ptr) + prefix_size,
                 b.length - prefix_size - sufix_size);
  }

public:
  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation =
      Allocator::supports_truncated_deallocation);

  typedef Allocator allocator;
  typedef Prefix prefix;
  typedef Sufix sufix;

  BOOST_STATIC_CONSTANT(unsigned, prefix_size =
      (std::is_same<Prefix, AffixAllocatorHelper::NoAffix>::value
          ? 0
          : sizeof(Prefix)));

  BOOST_STATIC_CONSTANT(unsigned, sufix_size =
      (std::is_same<Sufix, AffixAllocatorHelper::NoAffix>::value ? 0
                                                                : sizeof(Sufix)));

  AffixAllocator() {}

  AffixAllocator(AffixAllocator &&x) { *this = std::move(x); }

  AffixAllocator &operator=(AffixAllocator &&x) {
    _allocator = std::move(x._allocator);
    return *this;
  }

  /**
   * This Method returns on a given block the prefix.
   * \param b The block that was prefixed. The result is absolute unpredictable
   *          if a block is passed, that is not owned by this allocator!
   * \return Pointer to the Prefix before the given block
   */
  Prefix *outerToPrefix(const Block &b) const {
    return b ? reinterpret_cast<Prefix *>(static_cast<Prefix *>(b.ptr) - 1)
             : nullptr;
  }

  /**
  * This Method returns on a given block the sufix.
  * \param b The block that was sufixed. The result is absolute unpredictable
  *          if a block is passed, that is not owned by this allocator!
  * \return Pointer to the sufix before the given block
  */
  Sufix *outerToSufix(const Block &b) const {
    return b ? reinterpret_cast<Sufix *>(static_cast<char *>(b.ptr) + b.length)
             : nullptr;
  }

  /**
   * Allocates a Block of n bytes. Actually a Block of n + sizeof(Prefix) +
   * sizeof(Sufix) bytes is allocated. Depending of the defines Prefix and Sufix
   * types objects of this gets instantiated before and/or beyond the returned
   * Block. If Zero bytes are allocated then no allocation at all takes places
   * and an empty Block is returned.
   * \param n Specifies the number of requested bytes. n or more bytes are
   *          returned, depending on the alignment of the underlying Allocator.
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
   * The given block gets deallocated. If Prefix or Sufix are defined then
   * their d'tor(s) are called.
   * \param b The Block that should be freed. An assertion is raised, if the
   *          block is not owned by the underlying Allocator
   */
  void deallocate(Block &b) {
    if (!b) {
      return;
    }
    if (prefix_size > 0) {
      outerToPrefix(b)->~Prefix();
    }
    if (sufix_size > 0) {
      outerToSufix(b)->~Sufix();
    }
    auto innerBlock(toInnerBlock(b));
    _allocator.deallocate(innerBlock);
    b.reset();
  }

  /**
   * If the underlying Allocator defines ::owns() this method is available.
   * It returns true, if the given block is owned by this allocator.
   * \param b The Block that should be checked for ownership
   */
  typename traits::enable_result_to<bool,
                                    traits::has_owns<Allocator>::value>::type
  owns(const Block &b) const {
    return b && _allocator.owns(toInnerBlock(b));
  }

  /**
   * The given block gets reallocated to the new provided size n. Any potential
   * defined Prefix and/or Sufix gets copied to the new location.
   * \param b The block that should be resized
   * \param n The new size (n = zero means a deallocation)
   * \return True if the operation was successful
   */
  bool reallocate(Block &b, size_t n) {
    if (Helper::Reallocator<AffixAllocator>::isHandledDefault(*this, b, n)) {
      return true;
    }
    auto innerBlock = toInnerBlock(b);

    // Remember the old Sufix in case that it is available, because it must
    // be later placed to the new position
    boost::optional<Sufix> oldSufix;
    if (sufix_size > 0) {
      oldSufix.reset(*outerToSufix(b));
    }

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
   * The method tries to expand the given block by at least delta bytes insito
   * at the given location. This only This is only available if the underlaying
   * Allocator implements ::expand().
   * \param b The block that should be expanded
   * \param delta The number of bytes that the given block should be increased
   * \return True, if the operation was successful.
   */
  typename traits::enable_result_to<bool,
                                    traits::has_expand<Allocator>::value>::type
  expand(Block &b, size_t delta) {
    if (delta == 0) {
      return true;
    }

    if (!b) {
      b = allocate(delta);
      return static_cast<bool>(b);
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

private:

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
  AffixAllocator(const AffixAllocator &) = delete;
  AffixAllocator &operator=(const AffixAllocator &) = delete;
#endif
};

namespace traits {
/**
 * This trait implements a generic way to access a possible Affix surrounded
 * by a ALB::Block. In general it returns a nullptr. Only if the passed
 * Allocator is an AffixAllocator it returns a real object
 * \ingroup group_traits
 */
template <class Allocator, typename T> struct AffixExtractor {
  static T *prefix(Allocator &, const Block &) { return nullptr; }
  static T *sufix(Allocator &, const Block &) { return nullptr; }
};

template <class A, typename Prefix, typename Sufix, typename T>
struct AffixExtractor<AffixAllocator<A, Prefix, Sufix>, T> {
  static Prefix *prefix(AffixAllocator<A, Prefix, Sufix> &allocator,
                        const Block &b) {
    return allocator.outerToPrefix(b);
  }
  static Sufix *sufix(AffixAllocator<A, Prefix, Sufix> &allocator,
                      const Block &b) {
    return allocator.outerToSufix(b);
  }
};
}
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
