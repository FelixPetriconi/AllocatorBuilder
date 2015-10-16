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

#include <stddef.h>

namespace alb {
  namespace helpers {

    template <bool Used> uint64_t set_used(uint64_t const &currentRegister, uint64_t const &mask) noexcept;

    template <>
    inline uint64_t set_used<false>(uint64_t const &currentRegister, uint64_t const &mask) noexcept
    {
      return currentRegister & (mask ^ uint64_t(-1));
    }
    template <> inline uint64_t set_used<true>(uint64_t const &currentRegister, uint64_t const &mask) noexcept
    {
      return currentRegister | mask;
    }
  }
}