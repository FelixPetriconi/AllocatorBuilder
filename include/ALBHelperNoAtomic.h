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

namespace ALB {
namespace Helper {
  template <typename T>
  class NoAtomic {
    T _value;
  public:
    NoAtomic(T v) : _value(std::move(v)) {}

    T load() { return _value; }

    NoAtomic& operator=(T v) {
      _value = std::move(v);
      return *this;
    }

    bool compare_exchange_strong(T&, T v) {
      _value = std::move(v);
      return true;
    }
  };
}  
}