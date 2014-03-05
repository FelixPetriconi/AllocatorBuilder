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

#include <alb/allocator_base.hpp>
#include "TestHelpers/Algorithm.h"
#include <vector>

namespace alb {
namespace test_helpers {

const size_t ReferenceDataSize = 64;
const std::vector<int> ReferenceData = 
  []() -> std::vector<int> {
    std::vector<int> result(ReferenceDataSize);
    iota(std::begin(result), std::end(result), 0, 1);
    return result;
  }();



template <typename T>
void fillBlockWithReferenceData(alb::block& b)
{
  for (size_t i = 0; i < std::min(ReferenceData.size(), b.length / sizeof(T)); i++) {
    *(reinterpret_cast<T*>(b.ptr) + i) = ReferenceData[i];
  }
}

}
}