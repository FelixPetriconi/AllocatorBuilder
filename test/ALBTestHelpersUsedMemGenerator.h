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
#include <memory.h>
#include <future>
#include <array>
#include <stdlib.h>

#include "ALBAllocatorBase.h"
#include <vector>
#include <ALBMallocator.h>
#include <atomic>

namespace ALB {
namespace TestHelpers {

template <class Allocator, size_t BytesPerBitMarker>
class UsedMem
{
  Allocator& _allocator;
  std::vector<Block> _usedBlocks;    
  template <class A, size_t B>
  friend class UsedMemGenerator;

public:
  UsedMem(Allocator& allocator)
  : _allocator(allocator) {
  }

  const std::vector<Block>& blocks() const { return _usedBlocks; }
};

template <class Allocator, size_t BytesPerBitMarker>
class UsedMemGenerator {
  UsedMem<Allocator, BytesPerBitMarker> _usedMem;
  std::vector<Block> _freedLater;
public:
  UsedMemGenerator(Allocator& allocator)
    : _usedMem(allocator)
  {}

  UsedMemGenerator& withAUsedPatternOf(const char* pattern) {
    char* p = const_cast<char*>(pattern);
    while (*p) {
      if (*p == '1') { 
        _usedMem._usedBlocks.push_back(_usedMem._allocator.allocate(BytesPerBitMarker));
      }
      else if (*p == '0') {
        _freedLater.push_back(_usedMem._allocator.allocate(BytesPerBitMarker));
      }
      ++p;
    }
    return *this;
  }

  UsedMem<Allocator, BytesPerBitMarker> build() {
    for (auto& b : _freedLater) {
      _usedMem._allocator.deallocate(b);
    }
    return _usedMem;
  }

};

}
}