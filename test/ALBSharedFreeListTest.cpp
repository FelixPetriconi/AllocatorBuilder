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

#include "ALBTestHelpers.h"
#include "ALBSharedFreeList.h"
#include "ALBMallocator.h"

class SharedFreeListTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
    ALB::SharedFreeList<ALB::Mallocator, 0, 16 >>
{};

TEST_F(SharedFreeListTest, ThatASimpleAllocationReturnsAtLeastTheRequiredSize)
{
  auto mem = sut.allocate(16);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(16, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedFreeListTest, ThatADeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  auto mem = sut.allocate(8);
  auto oldPtr = mem.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem);

  mem = sut.allocate(8);
  EXPECT_EQ(oldPtr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedFreeListTest, ThatSeveralDeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);
  auto oldPtr1 = mem1.ptr;
  auto oldPtr2 = mem2.ptr;

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);

  mem1 = sut.allocate(8);
  mem2 = sut.allocate(8);

  EXPECT_EQ(oldPtr2, mem1.ptr);
  EXPECT_EQ(oldPtr1, mem2.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);
}

