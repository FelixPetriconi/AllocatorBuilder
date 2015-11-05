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
      template <typename T> 
      class no_atomic 
      {
        T value_;

      public:
        using type = T;

        no_atomic() noexcept
        {}

        explicit no_atomic(T v) noexcept
          : value_(std::move(v))
        {}

        T load() const noexcept {
          return value_;
        }

        no_atomic &operator=(T v) noexcept {
          value_ = std::move(v);
          return *this;
        }

        bool compare_exchange_strong(T &, T v) noexcept
        {
          value_ = std::move(v);
          return true;
        }

        operator T() const {
          return value_;
        }

        T operator++() { return ++value_; }
        T operator++(int) { return value_++; }
        T operator--() { return --value_;} 
        T operator--(int) { return value_--; }
        T operator+=(T arg) { return value_ += arg; }
        T operator-=(T arg) { return value_ -= arg; }
      };
    }
  }
  using namespace v_100;
}