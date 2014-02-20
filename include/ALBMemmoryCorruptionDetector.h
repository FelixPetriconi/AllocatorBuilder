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

#include <cassert>

namespace ALB
{
  template <typename T, size_t Pattern>
  class MemoryCorruptionDetector {
    static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(uint64_t), "Memory check not for valid types");
    
    T _pattern;
  public:
    typedef T value_type;
    static const T pattern = Pattern;

    MemoryCorruptionDetector() : _pattern(Pattern) {}

    ~MemoryCorruptionDetector() {
      BOOST_ASSERT(_pattern == Pattern);
    } 
  };
}