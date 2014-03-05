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
#include <memory>
#include <future>
#include <array>
#include <stdlib.h>
#include <vector>
#include <atomic>

#include <alb/allocator_base.hpp>
#include <alb/mallocator.hpp>

namespace alb
{
namespace test_helpers
{
  /**
    * Checks that both memory blocks are equal until n bytes
    */
  void EXPECT_MEM_EQ(void* a, void* b, size_t n);

  class TestMallocator{
    alb::mallocator _mallocator;
    static std::atomic<size_t> _allocatedMem;

  public:
    static const bool supports_truncated_deallocation = false;

    TestMallocator() {}

    block allocate(size_t n) {
      auto result = _mallocator.allocate(n);
      if (result) {
        _allocatedMem.fetch_add(result.length);
      }
      return result;
    }

    bool reallocate(block& b, size_t n) {
      _allocatedMem.fetch_sub(b.length);
      auto result = _mallocator.reallocate(b, n);
      _allocatedMem.fetch_add(b.length);
      return result;
    }

    void deallocate(block& b) {
      _allocatedMem.fetch_sub(b.length);
      _mallocator.deallocate(b);
    }

    static size_t currentlyAllocatedBytes() {
      return _allocatedMem.load();
    }
  };

}
}

