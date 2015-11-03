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

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4345)
#endif

namespace alb {
  inline namespace v_100 {
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

      /* simple optional store of a Sufix if the sufix_size > 0 */
      template <typename Sufix, size_t s>
      struct optinal_sufix_store
      {
        Sufix o_;
        void store(Sufix* o) noexcept {
          o_ = *o;
        }
        void unload(Sufix* o) noexcept {
          new (o) Sufix(o_);
        }
      };

      template <typename Sufix>
      struct optinal_sufix_store<Sufix, 0>
      {
        void store(Sufix*) {}
        void unload(Sufix*) {}
      };

    }

    /**
     * This allocator enables the possibility to surround allocated memory blocks
     * with guards, ref-counter, mutex or etc.
     * It is used by the alb::allocator_with_stats
     * It automatically places an object of type Prefix before the returned memory
     * location and an object of type Sufix after it. In case that they are of type
     * affix_allocatorHelper::Empty nothing is inserted. Depending on the alignment 
     * of the used Allocator, Prefix, memory and Sufix are each aligned.
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
      Allocator allocator_;

      Prefix *inner_to_prefix(const block &b) const noexcept
      {
        return static_cast<Prefix *>(b.ptr);
      }

      Sufix *inner_to_sufix(const block &b) const noexcept
      {
        return reinterpret_cast<Sufix*>(static_cast<char*>(b.ptr) + b.length - sufix_size);
      }

      block to_inner_block(const block &b) const noexcept
      {
        return{ static_cast<char *>(b.ptr) - prefix_size, b.length + prefix_size + sufix_size };
      }

      block to_outer_block(const block &b) const noexcept
      {
        return{ static_cast<char *>(b.ptr) + prefix_size, b.length - prefix_size - sufix_size };
      }

    public:
      affix_allocator(const affix_allocator &) = delete;
      affix_allocator &operator=(const affix_allocator &) = delete;

      static constexpr bool supports_truncated_deallocation = Allocator::supports_truncated_deallocation;
      static constexpr unsigned alignment = Allocator::alignment;

      using allocator = Allocator;
      using prefix = Prefix;
      using sufix = Sufix;

      static constexpr unsigned int prefix_size =
        std::is_same<Prefix, affix_allocator_helper::no_affix>::value ? 0 : sizeof(Prefix);

      static constexpr unsigned int sufix_size =
        std::is_same<Sufix, affix_allocator_helper::no_affix>::value ? 0 : sizeof(Sufix);

      static constexpr size_t good_size(size_t n) {
        return Allocator::good_size(n);
      }

      affix_allocator() noexcept
      {}

      affix_allocator(affix_allocator &&x) noexcept
      {
        *this = std::move(x);
      }

      affix_allocator &operator=(affix_allocator &&x) noexcept
      {
        allocator_ = std::move(x.allocator_);
        return *this;
      }

      /**
       * This Method returns on a given block the prefix.
       * \param b The block that was prefixed. The result is absolute unpredictable
       *          if a block is passed, that is not owned by this allocator!
       * \return Pointer to the Prefix before the given block
       */
      Prefix *outer_to_prefix(const block &b) const noexcept
      {
        return b ? (static_cast<Prefix *>(b.ptr) - 1) : nullptr;
      }

      /**
      * This Method returns on a given block the sufix.
      * \param b The block that was sufixed. The result is absolute unpredictable
      *          if a block is passed, that is not owned by this allocator!
      * \return Pointer to the sufix before the given block
      */
      Sufix *outer_to_sufix(const block &b) const noexcept
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
      block allocate(size_t n) noexcept
      {
        if (n == 0) {
          return{};
        }

        auto innerMem = allocator_.allocate(prefix_size + n + sufix_size);
        if (innerMem) {
          if (prefix_size > 0) {
            new (inner_to_prefix(innerMem)) Prefix{};
          }
          if (sufix_size > 0) {
            new (inner_to_sufix(innerMem)) Sufix{};
          }
          return to_outer_block(innerMem);
        }
        return{};
      }

      /**
       * The given block gets deallocated. If Prefix or Sufix are defined then
       * their d'tor(s) are called.
       * \param b The Block that should be freed.
       */
      void deallocate(block &b) noexcept
      {
        if (!b) {
          return;
        }
        if (prefix_size > 0) {
          outer_to_prefix(b)->~Prefix();
        }
        if (sufix_size > 0) {
          outer_to_sufix(b)->~Sufix();
        }
        auto innerBlock(to_inner_block(b));
        allocator_.deallocate(innerBlock);
        b.reset();
      }

      /**
       * If the underlying Allocator defines ::owns() this method is available.
       * It returns true, if the given block is owned by this allocator.
       * \param b The Block that should be checked for ownership
       */
      template <typename U = Allocator>
      typename std::enable_if<traits::has_owns<U>::value, bool>::type
        owns(const block &b) const noexcept
      {
        return b && allocator_.owns(to_inner_block(b));
      }

      /**
       * The given block gets reallocated to the new provided size n. Any potential
       * defined Prefix and/or Sufix gets copied to the new location.
       * \param b The block that should be resized
       * \param n The new size (n = zero means a deallocation)
       * \return True if the operation was successful
       */

      bool reallocate(block &b, size_t n) noexcept
      {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
          return true;
        }
        auto innerBlock = to_inner_block(b);

        // Remember the old Sufix in case that it is available, because it must
        // be later placed to the new position
        affix_allocator_helper::optinal_sufix_store<Sufix, sufix_size> oldSufix;
        oldSufix.store(outer_to_sufix(b));

        if (allocator_.reallocate(innerBlock, n + prefix_size + sufix_size)) {
          oldSufix.unload(inner_to_sufix(innerBlock));
          b = to_outer_block(innerBlock);
          return true;
        }
        return false;
      }

      /**
       * The method tries to expand the given block by at least delta bytes insito
       * at the given location. This is only available if the underlaying Allocator
       * implements ::expand().
       * \param b The block that should be expanded
       * \param delta The number of bytes that the given block should be increased
       * \return True, if the operation was successful.
       */
      template <typename U = Allocator>
      typename std::enable_if<traits::has_expand<U>::value, bool>::type
        expand(block &b, size_t delta) noexcept
      {
        if (delta == 0) {
          return true;
        }

        if (!b) {
          b = allocate(delta);
          return static_cast<bool>(b);
        }

        auto oldBlock = b;
        auto innerBlock = to_inner_block(b);

        if (allocator_.expand(innerBlock, delta)) {
          if (sufix_size > 0) {
            new (inner_to_sufix(innerBlock)) Sufix(*outer_to_sufix(oldBlock));
          }
          b = to_outer_block(innerBlock);
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
        static T *prefix(Allocator &, const block &) noexcept
        {
          return nullptr;
        }
        static T *sufix(Allocator &, const block &) noexcept
        {
          return nullptr;
        }
      };

      template <class A, typename Prefix, typename Sufix, typename T>
      struct affix_extractor<affix_allocator<A, Prefix, Sufix>, T> {
        static Prefix *prefix(affix_allocator<A, Prefix, Sufix> &allocator, const block &b) noexcept
        {
          return allocator.outer_to_prefix(b);
        }
        static Sufix *sufix(affix_allocator<A, Prefix, Sufix> &allocator, const block &b) noexcept
        {
          return allocator.outer_to_sufix(b);
        }
      };
    }
  }
  using namespace v_100;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
