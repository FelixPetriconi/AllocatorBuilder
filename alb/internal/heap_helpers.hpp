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
  namespace Helpers {

    template <bool Used> uint64_t setUsed(uint64_t const &currentRegister, uint64_t const &mask);

    template <>
    inline uint64_t setUsed<false>(uint64_t const &currentRegister, uint64_t const &mask) noexcept
    {
      return currentRegister & (mask ^ uint64_t(-1));
    }
    template <> inline uint64_t setUsed<true>(uint64_t const &currentRegister, uint64_t const &mask) noexcept
    {
      return currentRegister | mask;
    }
  }
}