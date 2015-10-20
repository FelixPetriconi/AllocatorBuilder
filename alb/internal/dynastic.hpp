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

#include <limits>

namespace alb {
  inline namespace v_100 {
    namespace internal {

      /**
       * Flag to be used inside the Dynastic struct to signal that the value
       * can be changed during runtime.
       * \ingroup group_internal
       */
      enum DynasticOptions : size_t {
        DynasticUndefined = std::numeric_limits<size_t>::max(),
        DynasticDynamicSet = std::numeric_limits<size_t>::max() - 1
      };

      /**
       * Simple generic value type that is either compile time constant or dynamically
       * set-able depending of DynamicEnableSwitch. If v and DynamicEnableSwitch, then
       * value can be changed during runtime.
       * @Author Andrei Alexandrescu
       *
       * \ingroup group_internal
       */
      template <size_t v, size_t DynamicEnableSwitch> struct dynastic {
        size_t value() const noexcept
        {
          return v;
        }
      };

      template <size_t DynamicEnableSwitch>
      struct dynastic<DynamicEnableSwitch, DynamicEnableSwitch> {
      private:
        size_t _v;

      public:
        dynastic() noexcept
          : _v(DynasticUndefined)
        {
        }

        size_t value() const noexcept
        {
          return _v;
        }

        void value(size_t w) noexcept
        {
          _v = w;
        }
      };

    } // namespace internal
  }

  using namespace v_100;
} // alb
