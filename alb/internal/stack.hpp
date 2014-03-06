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

#include <type_traits>
#include <boost/type_traits.hpp>

namespace alb {
namespace internal {

/**
 * Simple stack with the same interface as boost::lockfree::stack with a fixed
 * number of elements
 * It's intend is to be used as not shared variant in the FreeList
 * \tparam T The element type to be put into the stack
 * \tparam MaxSize The maximum number of elements that can be put into the stack
 *
 * \ingroup group_internal
 */
template <typename T, unsigned MaxSize> 
class stack {
  static_assert(boost::has_trivial_assign<T>::value,
    "T must be trivially copyable");
  static_assert(boost::has_trivial_destructor<T>::value,
    "T must be trivially destroyable");

  T _elements[MaxSize];
  int _pos;

public:
  typedef T value_type;
  static const size_t max_size = MaxSize;

  stack() : _pos(-1) {}

  bool push(T v) {
    if (_pos < static_cast<int>(MaxSize) - 1) {
      _pos++;
      _elements[_pos] = std::move(v);
      return true;
    }
    return false;
  }

  bool pop(T &v) {
    if (_pos >= 0) {
      v = std::move(_elements[_pos]);
      _pos--;
      return true;
    }
    return false;
  }

  bool empty() const { return _pos == -1; }
};
}
}