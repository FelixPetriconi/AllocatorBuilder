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

    /**
     * Simple stack with the same interface as boost::lockfree::stack.
     * It's intend is to be used as not shared variant in the FreeList
     */
    template <typename T, size_t MaxSize>
    class stack {
      T _elements[MaxSize];
      int _pos;
    public:
      stack() : _pos(-1) {}

      bool push(T v) {
        if (_pos < MaxSize - 1) {
          _pos++;
          _elements = std::move(v);
          return true;
        }
        return false;
      }

      bool pop(T& v) {
        if (_pos >= 0) {
          v = std::move(_elements[_pos]);
          _pos--;
          return true;
        }
        return false;
      }

      bool empty() const {
        return _pos == -1;
      }
    };
  }
}