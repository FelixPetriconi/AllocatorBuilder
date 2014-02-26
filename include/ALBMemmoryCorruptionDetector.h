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

namespace ALB {
/**
 * This class can be used as Prefix and/or Suffix with the AffixAllocator to
 * detect
 * buffer under-runs or overflows.
 * \tparam T must be an integral type. A guard with its size is used
 * \tparam Pattern this pattern is put into the memory as guard e.g. 0xdeadbeef
 * \ingroup group_helpers
 */
template <typename T, size_t Pattern> class MemoryCorruptionDetector {
  static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(uint64_t),
                "Memory check not for valid types");

  T _pattern;

public:
  typedef T value_type;
  static const T pattern = Pattern;

  MemoryCorruptionDetector() : _pattern(Pattern) {}

  ~MemoryCorruptionDetector() { BOOST_ASSERT(_pattern == Pattern); }
};
}