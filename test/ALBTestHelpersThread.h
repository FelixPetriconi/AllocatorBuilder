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

#include "allocator_base.hpp"
#include "ALBTestHelpersAlgorithm.h"
#include "ALBTestHelpersData.h"

#include <vector>

namespace ALB {
namespace TestHelpers {

/**
 * Class to check that no memory overrun takes place with allocations, reallocations 
 * and deallocations.
 * Use check() to start the test.
 */
template <class Allocator>
class TestWorker
{
  std::unique_ptr<char[]> _reference;
  Allocator& _allocator;
  const int _maxUsedBytes;

  void init() {
    _reference.reset(new char[_maxUsedBytes]);
    unsigned char pattern = ::rand() * 255 / RAND_MAX;
    ::memset(_reference.get(), pattern, _maxUsedBytes);
  }

public:
  TestWorker(Allocator& heap, int maxUsedBytes) 
    : _allocator(heap)
    , _maxUsedBytes(maxUsedBytes) {
    init();
  }


  void setReady(std::promise<void>& workerReady) {
    workerReady.set_value();
  }

  void check() {
    Oszillator bytesToWork(_maxUsedBytes);

    for (auto i = 0; i < 100000; i++) {
      auto mem = _allocator.allocate(bytesToWork);
      if (mem) {
        ::memcpy(mem.ptr, _reference.get(), bytesToWork);
        EXPECT_MEM_EQ(mem.ptr, _reference.get(), bytesToWork);
      }
      if (bytesToWork < _maxUsedBytes / 2) {
        if (_allocator.reallocate(mem, bytesToWork * 2)) {
          EXPECT_MEM_EQ(mem.ptr, _reference.get(), bytesToWork);
        }
      }
      else {
        if (_allocator.reallocate(mem, bytesToWork / 2)) {
          EXPECT_MEM_EQ(mem.ptr, _reference.get(), bytesToWork / 2);
        }
      }

      _allocator.deallocate(mem);

      bytesToWork++;
    }
  }
};


/**
 * Class to check that no memory overrun takes place with while it holds multiple allocations 
 * at the same time 
 * Use check() to start the test.
 */
template <class Allocator>
class MultipleAllocationsTester
{
  std::unique_ptr<char[]> _reference;
  Allocator& _allocator;
  const int _maxUsedBytes;
  
  void init() {
    _reference.reset(new char[_maxUsedBytes]);
    unsigned char pattern = ::rand() * 255 / RAND_MAX;
    ::memset(_reference.get(), pattern, _maxUsedBytes);
  }

public:
  MultipleAllocationsTester(Allocator& heap, int maxUsedBytes) 
    : _allocator(heap)
    , _maxUsedBytes(maxUsedBytes) {
      init();
  }


  void setReady(std::promise<void>& workerReady) {
    workerReady.set_value();
  }

  void check() {
    Oszillator bytesToWork(_maxUsedBytes);
    Oszillator memBlockIndex(7);
    std::vector<ALB::Block> mems;

    for (size_t i = 0; i < 8; i++) {
      auto mem = _allocator.allocate(bytesToWork++);
      ::memcpy(mem.ptr, _reference.get(), bytesToWork);
      mems.push_back(mem);
    }

    for (auto i = 0; i < 100000; i++) {
      if (_allocator.reallocate(mems[memBlockIndex], bytesToWork)) {
        if (mems[memBlockIndex]) {
          ::memcpy(mems[memBlockIndex].ptr, _reference.get(), bytesToWork);
          EXPECT_MEM_EQ(mems[memBlockIndex].ptr, _reference.get(), bytesToWork);
        }
      }
      bytesToWork++;
      memBlockIndex++;
      _allocator.deallocate(mems[memBlockIndex]);

      mems[memBlockIndex] = _allocator.allocate(bytesToWork);
      ::memcpy(mems[memBlockIndex].ptr, _reference.get(), bytesToWork);

      memBlockIndex++;
    }
    
    for (size_t i = 0; i < 8; i++) {
      _allocator.deallocate(mems[i]);
    }
  }
};


/**
 * Class that lets run in ThreadNumber threads Tester instances on the given Allocator
 * All threads are started, Testers initialization is done and wait until all Testers 
 * are ready. Then all threads are started in parallel to ensure that the Allocator is
 * really stressed by different threads
 */
template <class Allocator, size_t ThreadNumber, class Tester, class TestParams>
class TestWorkerCollector
{
  Allocator& _allocator;
  std::promise<void> _go;
  std::shared_future<void> _ready;

  struct TestWorkerContext
  {
    std::promise<void> ready;
    std::future<void> done;
    std::unique_ptr<Tester> tester;
  };

  std::array<TestWorkerContext, ThreadNumber> _workerContexts;

  void waitAllReady()
  {
    for (auto& context : _workerContexts) {
      context.ready.get_future().wait();
    }
  }

  void waitAllDone() {
    for (auto& context : _workerContexts) {
      context.done.get();
    }
  }

public:
  TestWorkerCollector(Allocator& allocator, const TestParams& params) 
    : _allocator(allocator)
    , _ready(_go.get_future())
  {
    for (size_t i = 0; i < ThreadNumber; i++) {
      _workerContexts[i].tester.reset( new Tester(_allocator, params[i]) );
      _workerContexts[i].done = std::async(std::launch::async,
        [this, i]() {
          _workerContexts[i].tester->setReady(_workerContexts[i].ready);
          _ready.wait();
          _workerContexts[i].tester->check();
      });
    }
  }

  void check()
  {
    waitAllReady();
    _go.set_value();
    waitAllDone();
  }
};

}
}