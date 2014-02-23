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
#include "ALBStackAllocator.h"

class SharedFreeListTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
    ALB::SharedFreeList<ALB::Mallocator, 0, 16 >>
{
protected:
  void TearDown() {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
  ALB::Block mem;
};

TEST_F(SharedFreeListTest, ThatASimpleAllocationReturnsAtLeastTheRequiredSize)
{
  mem = sut.allocate(16);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(16, mem.length);
}

TEST_F(SharedFreeListTest, ThatADeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  mem = sut.allocate(8);
  auto oldPtr = mem.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem);

  mem = sut.allocate(8);
  EXPECT_EQ(oldPtr, mem.ptr);
}

TEST_F(SharedFreeListTest, ThatSeveralDeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);
  auto oldPtr1 = mem1.ptr;
  auto oldPtr2 = mem2.ptr;

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);

  mem1 = sut.allocate(12);
  mem2 = sut.allocate(12);

  EXPECT_EQ(oldPtr2, mem1.ptr);
  EXPECT_EQ(oldPtr1, mem2.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);
}

TEST_F(SharedFreeListTest, ThatReallocatingAnEmptyBlockResultsToBlockOfBoundsSize)
{
  EXPECT_TRUE(sut.reallocate(mem, 8));
  EXPECT_EQ(16, mem.length);
}


TEST_F(SharedFreeListTest, ThatReallocatingAFilledBlockToNonZeroIsRejected)
{
  mem = sut.allocate(8);
  EXPECT_FALSE(sut.reallocate(mem, 2));
  EXPECT_EQ(16, mem.length);
}

TEST_F(SharedFreeListTest, ThatBlocksOutsideTheBoundsAreNotRecognizedAsOwned)
{
  EXPECT_FALSE(sut.owns(ALB::Block()));
  EXPECT_FALSE(sut.owns(ALB::Block(nullptr, 64)));

  ALB::SharedFreeList<ALB::Mallocator, 16, 32> sutNotStartingAtZero;
  EXPECT_FALSE(sutNotStartingAtZero.owns(ALB::Block(nullptr, 15)));
  EXPECT_FALSE(sutNotStartingAtZero.owns(ALB::Block(nullptr, 33)));
}

TEST_F(SharedFreeListTest, ThatAProvidedBlockIsRecongizedAOwned)
{
  mem = sut.allocate(16);
  EXPECT_TRUE(sut.owns(mem));
}


TEST(SharedFreeListWithParametrizedTest, ThatUpperAndLowerBoundIsSet)
{
  ALB::SharedFreeList<ALB::Mallocator, ALB::DynamicSetSize, ALB::DynamicSetSize> sut(16, 42);
  EXPECT_EQ(16, sut.min_size());
  EXPECT_EQ(42, sut.max_size());
}

TEST(SharedFreeListWithParametrizedTest, ThatAllocationsBeyondTheBoundariesAreRejected)
{
  ALB::SharedFreeList<ALB::Mallocator, ALB::DynamicSetSize, ALB::DynamicSetSize> sut(16, 42);
  size_t rejectedValues[] = { 0, 4, 15, 43, 64 };
  for (auto i : rejectedValues) {
    auto mem = sut.allocate(i);
    EXPECT_FALSE(mem);
  }
}

TEST(SharedFreeListWithTruncatedDeallocationParentAllocatorTest, ThatThePreAllocatedBlocksAreInLine)
{
  ALB::SharedFreeList<ALB::StackAllocator<1024>, 0, 16,1024, 4> sut;
  ALB::Block mem[8]; // 0 3 2 1  0 3 2 1 order of allocations
  mem[0] = sut.allocate(16);
  for (size_t i = 3; i > 0; i--) {
    mem[i] = sut.allocate(16);
  }
  mem[4] = sut.allocate(16);
  for (size_t i = 7; i > 4; i--) {
    mem[i] = sut.allocate(16);
  } // 0 1 2 3 0 1 2 3
  for (size_t i = 0; i < 7; i++) {
    EXPECT_EQ(static_cast<char*>(mem[i].ptr) + 16, mem[i + 1].ptr) << "Failure at " << i;
  }
}
