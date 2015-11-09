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

#include <stddef.h>

namespace alb {
  inline namespace v_100 {
    namespace affix_helper {

      template <typename Affix, typename Enabled = void>
      struct affix_creator;

      template <typename Affix>
      struct affix_creator<Affix, typename std::enable_if<std::is_default_constructible<Affix>::value>::type>
      {
        template <typename Allocator>
        static constexpr void doIt(void *p, Allocator&)
        {
          new (p) Affix{};
        }
      };

      template <typename Affix>
      struct affix_creator<Affix, typename std::enable_if<!std::is_default_constructible<Affix>::value>::type>
      {
        template <typename Allocator>
        static constexpr void doIt(void *p, Allocator& a)
        {
          new (p) Affix(a);
        }
      };

      template <typename Affix, typename Allocator>
      void create_affix_in_place(void *p, Allocator& a)
      {
        affix_creator<Affix>::doIt(p, a);
      }


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
  }
  using namespace v_100;

}