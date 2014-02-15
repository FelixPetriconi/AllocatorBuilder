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

namespace ALB
{
  namespace TestHelpers
  {
    template <class Allocator>
    class AllocatorBaseTest : public ::testing::Test
    {
    protected:
      void deallocateAndCheckBlockIsThenEmpty(ALB::Block& b) {
        sut.deallocate(b);
        EXPECT_FALSE(b);
        EXPECT_EQ(nullptr, b.ptr);
      }
      Allocator sut;
    };

    /**
     * Simple class that oscillates from zero to maxValue with each increment
     */
    class Oszillator
    {
      int _value;
      bool _up;
      int _maxValue;
    public:
      Oszillator(int maxValue)
        : _value(0)
        , _up(true)
        , _maxValue(maxValue)
      {}

      operator int(){
        return _value;
      }

      Oszillator operator++(int)
      {
        if (_up) {
          ++_value;
        }
        else {
          --_value;
        }
        if (_value == _maxValue) {
          _up = false;
        }
        else if (_value == 0) {
          _up = true;
        }
        return *this;
      }
    };



    template <typename I, typename N> 
    N iota(I first, I last, N start, N step) {
      typedef typename std::iterator_traits<I>::value_type T;
      while (first != last) {
        *first = T(start);
        start += step;
        ++first;
      }
      return start;
    }



    const size_t ReferenceDataSize = 64;
    const std::vector<int> ReferenceData = 
      []() -> std::vector<int> {
        std::vector<int> result(ReferenceDataSize);
        iota(std::begin(result), std::end(result), 0, 1);
        return result;
      }();



    template <typename T>
    void fillBlockWithReferenceData(ALB::Block& b)
    {
      for (size_t i = 0; i < std::min(ReferenceData.size(), b.length / sizeof(T)); i++) {
        *(reinterpret_cast<T*>(b.ptr) + i) = ReferenceData[i];
      }
    }


    /**
     * Checks that both memory blocks are equal until n bytes
     */
    void EXPECT_MEM_EQ(void* a, void* b, size_t n);


    template <typename T, size_t Pattern>
    class AffixGuard {
      static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(uint64_t), "Memory check not for valid types");

      T _pattern;
    public:
      typedef T value_type;
      static const T pattern = Pattern;

      AffixGuard() : _pattern(Pattern) {}

      AffixGuard(const AffixGuard& o) : _pattern(o._pattern) {
        EXPECT_EQ(_pattern, Pattern);
      }
      ~AffixGuard() {
        EXPECT_EQ(_pattern, Pattern);
      } 
    };
    /**
     * Class to check that no memory overrun takes place with allocations, reallocations 
     * and deallocations.
     * Use check() to start the test.
     */
    template <class Allocator>
    class TestWorker
    {
      std::unique_ptr<char[]> _reference;
      const int _maxUsedBytes;
      Allocator& _allocator;

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
      const int _maxUsedBytes;
      Allocator& _allocator;

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

