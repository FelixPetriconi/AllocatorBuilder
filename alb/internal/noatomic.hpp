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

namespace alb {
  inline namespace v_100 {
    namespace internal {

      /**
        * Template that mimics (partly) the std::atomic<T> interface, without
        * beeing an atomic.
        * It is usefull, if during compilation time the selection between single
        * threaded or multi threaded is needed
        * \tparam T The value that the class encapsulate
        *
        * \ingroup group_internal
        */
      template <typename T> class NoAtomic {
        T _value;

      public:
        NoAtomic() noexcept
        {
        }

        explicit NoAtomic(T v) noexcept
          : _value(std::move(v))
        {
        }

        T load() const noexcept
        {
          return _value;
        }

        NoAtomic &operator=(T v) noexcept
        {
          _value = std::move(v);
          return *this;
        }

        bool compare_exchange_strong(T &, T v) noexcept
        {
          _value = std::move(v);
          return true;
        }

        operator T() const
        {
          return _value;
        }
      };
    }
  }
  using namespace v_100;
}