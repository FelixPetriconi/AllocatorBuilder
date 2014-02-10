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
#include <gmock/gmock.h>
#include "ALBSharedCascadingAllocators.h"
#include "ALBSharedHeap.h"
#include "ALBTestHelpers.h"
#include "ALBMallocator.h"
#include <thread>
#include <future>

class TCascadingAllocatorsTest : public ::testing::Test
{
};

TEST_F(TCascadingAllocatorsTest, SingleAllocation)
{
  ALB::SharedCascadingAllocators<ALB::SharedHeap<ALB::Mallocator<>, 64,8>> sut;

  auto m1 = sut.allocate(64*8);
  auto m2 = sut.allocate(64*8);

  sut.deallocate(m2);
  sut.deallocate(m1);
}

TEST_F(TCascadingAllocatorsTest,  BruteForceAllocationByOneRunningThread)
{
  typedef ALB::SharedCascadingAllocators<ALB::SharedHeap<ALB::Mallocator<>, 64,8>> AllocatorUnderTest;
  AllocatorUnderTest sut;

  ALB::TestHelpers::TestWorker<AllocatorUnderTest> testWorker(sut, 256);
  testWorker.check();
}


TEST_F(TCascadingAllocatorsTest,  BruteForceSingleAllocationWithinTwoRunningThreads)
{
  typedef ALB::SharedCascadingAllocators<ALB::SharedHeap<ALB::Mallocator<>, 512,64>> AllocatorUnderTest;
  AllocatorUnderTest sut;

  typedef std::array<unsigned char, 2> TestParams;
  TestParams maxUsedBytes = {127, 129};

  ALB::TestHelpers::TestWorkerCollector<
    AllocatorUnderTest, 
    2, 
    ALB::TestHelpers::TestWorker<AllocatorUnderTest>,
    TestParams> singleMemoryAccessTest(sut, maxUsedBytes);

  singleMemoryAccessTest.check();  

  auto allDeallocatedCheck = sut.allocate(512*64);
  EXPECT_TRUE((bool)allDeallocatedCheck);
  sut.deallocate(allDeallocatedCheck);
}

TEST_F(TCascadingAllocatorsTest,  BruteForceWithSeveralAllocatedBlocksWithinTwoRunningThreads)
{
  typedef ALB::SharedCascadingAllocators<ALB::SharedHeap<ALB::Mallocator<>, 512,64>> AllocatorUnderTest;
  AllocatorUnderTest sut;

  typedef std::array<unsigned char, 2> TestParams;
  TestParams maxUsedBytes = {127, 129};

  ALB::TestHelpers::TestWorkerCollector<
    AllocatorUnderTest, 
    2, 
    ALB::TestHelpers::TestWorker<AllocatorUnderTest>,
    TestParams> multipleMemoryAccessTest(sut, maxUsedBytes);

  multipleMemoryAccessTest.check();

  auto allDeallocatedCheck = sut.allocate(512*64);
  EXPECT_TRUE((bool)allDeallocatedCheck);
  sut.deallocate(allDeallocatedCheck);
}
