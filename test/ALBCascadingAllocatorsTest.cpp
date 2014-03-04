///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
///////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>
#include <alb/cascading_allocator.hpp>
#include <alb/shared_heap.hpp>
#include <alb/mallocator.hpp>
#include "ALBTestHelpersThread.h"
#include "ALBTestHelpers.h"

#include <thread>
#include <future>

class TCascadingAllocatorsTest : public ::testing::Test
{
};

TEST_F(TCascadingAllocatorsTest, SingleAllocation)
{
  alb::shared_cascading_allocator<alb::shared_heap<alb::mallocator, 64,8>> sut;

  auto m1 = sut.allocate(64*8);
  auto m2 = sut.allocate(64*8);

  sut.deallocate(m2);
  sut.deallocate(m1);
}

TEST_F(TCascadingAllocatorsTest,  BruteForceAllocationByOneRunningThread)
{
  typedef alb::shared_cascading_allocator<alb::shared_heap<alb::mallocator, 512,64>> AllocatorUnderTest;
  AllocatorUnderTest sut;

  alb::test_helpers::TestWorker<AllocatorUnderTest> testWorker(sut, 129);
  testWorker.check();
}


TEST_F(TCascadingAllocatorsTest,  BruteForceSingleAllocationWithinTwoRunningThreads)
{
  typedef alb::shared_cascading_allocator<alb::shared_heap<alb::mallocator, 512,64>> AllocatorUnderTest;
  AllocatorUnderTest sut;
  const size_t NumberOfThreads = 4;

  typedef std::array<unsigned char, NumberOfThreads> TestParams;
  TestParams maxUsedBytes = {127, 129, 65, 130};

  alb::test_helpers::TestWorkerCollector<
    AllocatorUnderTest, 
    NumberOfThreads, 
    alb::test_helpers::TestWorker<AllocatorUnderTest>,
    TestParams> singleMemoryAccessTest(sut, maxUsedBytes);

  singleMemoryAccessTest.check();  
}

TEST_F(TCascadingAllocatorsTest,  BruteForceWithSeveralAllocatedBlocksWithinTwoRunningThreads)
{
  typedef alb::shared_cascading_allocator<alb::shared_heap<alb::mallocator, 512,64>> AllocatorUnderTest;
  AllocatorUnderTest sut;
  const size_t NumberOfThreads = 4;

  typedef std::array<unsigned char, NumberOfThreads> TestParams;
  TestParams maxUsedBytes = {127, 129, 65, 130};

  alb::test_helpers::TestWorkerCollector<
    AllocatorUnderTest, 
    NumberOfThreads, 
    alb::test_helpers::TestWorker<AllocatorUnderTest>,
    TestParams> multipleMemoryAccessTest(sut, maxUsedBytes);

  multipleMemoryAccessTest.check();
}
