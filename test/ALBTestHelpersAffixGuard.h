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

#include <gtest/gtest.h>

namespace ALB {
namespace TestHelpers {

template <typename T, size_t Pattern>
class AffixGuard {
  static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(uint64_t), "Memory check not for valid types");

  T _pattern;
public:
  typedef T value_type;
  static const T pattern = Pattern;

  AffixGuard() : _pattern(Pattern) {}

  AffixGuard(const AffixGuard& o) : _pattern(o._pattern) {
    EXPECT_EQ(_pattern, Pattern);
  }
  ~AffixGuard() {
    EXPECT_EQ(_pattern, Pattern);
  } 
};
}
}