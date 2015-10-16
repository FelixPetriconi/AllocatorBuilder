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

#include <alb/allocator_base.hpp>
#include <vector>
#include <alb/mallocator.hpp>
#include <atomic>

namespace alb {
namespace test_helpers {

template <class Allocator, size_t BytesPerBitMarker>
class UsedMem
{
  Allocator& allocator_;
  std::vector<block> _usedBlocks;    
  template <class A, size_t B>
  friend class UsedMemGenerator;

public:
  UsedMem(Allocator& allocator)
  : allocator_(allocator) {
  }

  const std::vector<block>& blocks() const { return _usedBlocks; }
};

template <class Allocator, size_t BytesPerBitMarker>
class UsedMemGenerator {
  UsedMem<Allocator, BytesPerBitMarker> _usedMem;
  std::vector<block> _freedLater;
public:
  UsedMemGenerator(Allocator& allocator)
    : _usedMem(allocator)
  {}

  UsedMemGenerator& withAUsedPatternOf(const char* pattern) {
    char* p = const_cast<char*>(pattern);
    while (*p) {
      if (*p == '1') { 
        _usedMem._usedBlocks.push_back(_usedMem.allocator_.allocate(BytesPerBitMarker));
      }
      else if (*p == '0') {
        _freedLater.push_back(_usedMem.allocator_.allocate(BytesPerBitMarker));
      }
      ++p;
    }
    return *this;
  }

  UsedMem<Allocator, BytesPerBitMarker> build() {
    for (auto& b : _freedLater) {
      _usedMem.allocator_.deallocate(b);
    }
    return _usedMem;
  }

};

}
}