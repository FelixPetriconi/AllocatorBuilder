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

#include "allocator_base.hpp"
#include "internal/reallocator.hpp"
#include <boost/optional.hpp>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4345)
#endif

namespace alb {
  namespace affix_allocator_helper {
    /**
     * This special kind of no_affix is necessary to have the possibility to test
     * the
     * affix_allocator in a generic way. This must be used to disable a Prefix or
     * Sufix.
     */
    struct no_affix {
      using value_type = int;
      static const int pattern = 0;
    };
  }

  /**
   * This allocator enables the possibility to surround allocated memory blocks
   * with guards, ref-counter, mutex or etc.
   * It is used by the alb::allocator_with_stats
   * It automatically places an object of type Prefix before the returned memory
   * location and an object of type Sufix after it. In case that they are of type
   * affix_allocatorHelper::Empty nothing is inserted.
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
  template <class Allocator, typename Prefix, typename Sufix = affix_allocator_helper::no_affix>
  class affix_allocator {
    Allocator _allocator;

    Prefix *innerToPrefix(const block &b) const
    {
      return static_cast<Prefix *>(b.ptr);
    }

    Sufix *innerToSufix(const block &b) const
    {
      return reinterpret_cast<Sufix *>(static_cast<char *>(b.ptr) + b.length - sufix_size);
    }

    block toInnerBlock(const block &b) const
    {
      return {static_cast<char *>(b.ptr) - prefix_size, b.length + prefix_size + sufix_size};
    }

    block toOuterBlock(const block &b) const
    {
      return {static_cast<char *>(b.ptr) + prefix_size, b.length - prefix_size - sufix_size};
    }

  public:
    affix_allocator(const affix_allocator &) = delete;
    affix_allocator &operator=(const affix_allocator &) = delete;

    static const bool supports_truncated_deallocation = Allocator::supports_truncated_deallocation;

    using allocator = Allocator;
    using prefix = Prefix;
    using sufix = Sufix;

    static const unsigned int prefix_size =
        std::is_same<Prefix, affix_allocator_helper::no_affix>::value ? 0 : sizeof(Prefix);

    static const unsigned int sufix_size =
        std::is_same<Sufix, affix_allocator_helper::no_affix>::value ? 0 : sizeof(Sufix);

    affix_allocator()
    {
    }

    affix_allocator(affix_allocator &&x)
    {
      *this = std::move(x);
    }

    affix_allocator &operator=(affix_allocator &&x)
    {
      _allocator = std::move(x._allocator);
      return *this;
    }

    /**
     * This Method returns on a given block the prefix.
     * \param b The block that was prefixed. The result is absolute unpredictable
     *          if a block is passed, that is not owned by this allocator!
     * \return Pointer to the Prefix before the given block
     */
    Prefix *outerToPrefix(const block &b) const
    {
      return b ? (static_cast<Prefix *>(b.ptr) - 1) : nullptr;
    }

    /**
    * This Method returns on a given block the sufix.
    * \param b The block that was sufixed. The result is absolute unpredictable
    *          if a block is passed, that is not owned by this allocator!
    * \return Pointer to the sufix before the given block
    */
    Sufix *outerToSufix(const block &b) const
    {
      return b ? (reinterpret_cast<Sufix*>(static_cast<char *>(b.ptr) + b.length)) : nullptr;
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
    block allocate(size_t n)
    {
      if (n == 0) {
        return {};
      }

      auto innerMem = _allocator.allocate(prefix_size + n + sufix_size);
      if (innerMem) {
        if (prefix_size > 0) {
          new (innerToPrefix(innerMem)) Prefix{};
        }
        if (sufix_size > 0) {
          new (innerToSufix(innerMem)) Sufix{};
        }
        return toOuterBlock(innerMem);
      }
      return {};
    }

    /**
     * The given block gets deallocated. If Prefix or Sufix are defined then
     * their d'tor(s) are called.
     * \param b The Block that should be freed. An assertion is raised, if the
     *          block is not owned by the underlying Allocator
     */
    void deallocate(block &b)
    {
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
    template <typename U = Allocator>
    typename std::enable_if<traits::has_owns<U>::value, bool>::type
    owns(const block &b) const
    {
      return b && _allocator.owns(toInnerBlock(b));
    }

    /**
     * The given block gets reallocated to the new provided size n. Any potential
     * defined Prefix and/or Sufix gets copied to the new location.
     * \param b The block that should be resized
     * \param n The new size (n = zero means a deallocation)
     * \return True if the operation was successful
     */
    bool reallocate(block &b, size_t n)
    {
      if (internal::reallocator<affix_allocator>::isHandledDefault(*this, b, n)) {
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
    template <typename U = Allocator>
    typename std::enable_if<traits::has_expand<U>::value, bool>::type
    expand(block &b, size_t delta)
    {
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
  };

  namespace traits {
    /**
     * This trait implements a generic way to access a possible Affix surrounded
     * by a ALB::Block. In general it returns a nullptr. Only if the passed
     * Allocator is an affix_allocator it returns a real object
     * \ingroup group_traits
     */
    template <class Allocator, typename T> struct affix_extractor {
      static T *prefix(Allocator &, const block &)
      {
        return nullptr;
      }
      static T *sufix(Allocator &, const block &)
      {
        return nullptr;
      }
    };

    template <class A, typename Prefix, typename Sufix, typename T>
    struct affix_extractor<affix_allocator<A, Prefix, Sufix>, T> {
      static Prefix *prefix(affix_allocator<A, Prefix, Sufix> &allocator, const block &b)
      {
        return allocator.outerToPrefix(b);
      }
      static Sufix *sufix(affix_allocator<A, Prefix, Sufix> &allocator, const block &b)
      {
        return allocator.outerToSufix(b);
      }
    };
  }
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
