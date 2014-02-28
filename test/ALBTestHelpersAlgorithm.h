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
namespace TestHelpers {

/**
 * Simple class that oscillates from zero to maxValue with each increment
 */
class Oszillator
{
  int _value;
  bool _up;
  int _maxValue;
public:
  Oszillator(int maxValue)
    : _value(0)
    , _up(true)
    , _maxValue(maxValue)
  {}

  operator int(){
    return _value;
  }

  Oszillator operator++(int)
  {
    if (_up) {
      ++_value;
    }
    else {
      --_value;
    }
    if (_value == _maxValue) {
      _up = false;
    }
    else if (_value == 0) {
      _up = true;
    }
    return *this;
  }
};



template <typename I, typename N> 
N iota(I first, I last, N start, N step) {
  typedef typename std::iterator_traits<I>::value_type T;
  while (first != last) {
    *first = T(start);
    start += step;
    ++first;
  }
  return start;
}
}

}