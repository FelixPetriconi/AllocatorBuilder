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

#include <boost/assert.hpp>
#include <boost/config/suffix.hpp>

namespace ALB {
/**
 * This class can be used as Prefix and/or Suffix with the AffixAllocator to
 * detect
 * buffer under-runs or overflows.
 * \tparam T must be an integral type. A guard with its size is used
 * \tparam Pattern this pattern is put into the memory as guard e.g. 0xdeadbeef
 *
 * \ingroup group_helpers
 */
template <typename T, size_t Pattern> class MemoryCorruptionDetector {
  static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(uint64_t),
                "Memory check not for supported types");

  T _pattern;

public:
  typedef T value_type;
  BOOST_STATIC_CONSTANT(size_t, pattern = Pattern);

  MemoryCorruptionDetector() : _pattern(Pattern) {}

  ~MemoryCorruptionDetector() { BOOST_ASSERT(_pattern == Pattern); }
};

template <typename T, size_t Pattern>
const size_t MemoryCorruptionDetector<T,Pattern>::pattern;

}