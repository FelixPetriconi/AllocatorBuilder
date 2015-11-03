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
#include <cstdint>

namespace alb {
  inline namespace v_100 {
    /**
     * This class can be used as Prefix and/or Suffix with the affix_allocator to
     * detect
     * buffer under-runs or overflows.
     * \tparam T must be an integral type. A guard with its size is used
     * \tparam Pattern this pattern is put into the memory as guard e.g. 0xdeadbeef
     *
     * \ingroup group_internal
     */
    template <typename T, size_t Pattern> 
    class memory_corruption_detector 
    {
      static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(uint64_t),
        "Memory check not for supported types");

      T pattern_;

    public:
      using value_type = T;
      static const size_t pattern = Pattern;

      memory_corruption_detector() noexcept
        : pattern_(Pattern)
      {}

      ~memory_corruption_detector()
      {
        assert(pattern_ == Pattern);
      }
    };

    template <typename T, size_t Pattern>
    const size_t memory_corruption_detector<T, Pattern>::pattern;
  }
  using namespace v_100;
}