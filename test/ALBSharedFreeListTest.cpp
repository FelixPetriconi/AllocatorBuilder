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

#include "ALBSharedFreeList.h"
#include "ALBMallocator.h"

TEST(TFreeListTest, ThatASimpleAllocationReturnsAtLeastTheRequiredSize)
{
  ALB::SharedFreeList<ALB::Mallocator, 0, 17 > sut;
  auto mem = sut.allocate(8);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(16, mem.length);

  sut.deallocate(mem);
}

TEST(TFreeListTest, ThatADeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  ALB::SharedFreeList<ALB::Mallocator, 0, 17> sut;
  auto mem = sut.allocate(8);
  auto oldPtr = mem.ptr;
  sut.deallocate(mem);

  mem = sut.allocate(8);
  EXPECT_EQ(oldPtr, mem.ptr);

  sut.deallocate(mem);
}

TEST(TFreeListTest, ThatSeveralDeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  ALB::SharedFreeList<ALB::Mallocator, 0, 17> sut;
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);
  auto oldPtr1 = mem1.ptr;
  auto oldPtr2 = mem2.ptr;

  sut.deallocate(mem1);
  sut.deallocate(mem2);

  mem1 = sut.allocate(8);
  mem2 = sut.allocate(8);

  EXPECT_EQ(oldPtr2, mem1.ptr);
  EXPECT_EQ(oldPtr1, mem2.ptr);

  sut.deallocate(mem1);
  sut.deallocate(mem2);
}

