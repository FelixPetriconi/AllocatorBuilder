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

#include "ALBStackAllocator.h"
#include "ALBTestHelpers.h"
#include "ALBTestHelpersAllocatorBaseTest.h"

class StackAllocatorTest: public ALB::TestHelpers::AllocatorBaseTest<ALB::StackAllocator<64,4>>
{
};

TEST_F(StackAllocatorTest, ThatAllocatingZeroBytesReturnsAnEmptyMemoryBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatAllocatingOneBytesReturnsABlockOfOneByte)
{
  auto mem = sut.allocate(1);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(4, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatAllocatingTheCompleteStackSizeIsPossible)
{
  auto mem = sut.allocate(64);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(64, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}


TEST_F(StackAllocatorTest, ThatAllocatingTwoMemoryBlocksUsesContiguousMemory)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);
  EXPECT_EQ(mem2.ptr, static_cast<char*>(mem1.ptr) + 8);
  EXPECT_EQ(8, mem1.length);
  EXPECT_EQ(8, mem2.length);

  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem1);
}

TEST_F(StackAllocatorTest, ThatAFreedBlockWhichWasTheLastAllocatedOnesGetsReusedForANewAllocation)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);

  auto ptrOf2ndLocation = mem2.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem2);

  auto mem3 = sut.allocate(8);
  EXPECT_EQ(ptrOf2ndLocation, mem3.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem3);
}

TEST_F(StackAllocatorTest, ThatAFreedBlockWhichWasNotTheLastAllocatedOnesGetsNotReusedForANewAllocation)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);

  auto ptrOf1stLocation = mem1.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem1);

  auto mem3 = sut.allocate(8);
  EXPECT_NE(ptrOf1stLocation, mem3.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem3);
}

TEST_F(StackAllocatorTest, ThatANullBlockIsReturnedWhenTheAllocatorIsOutOfMemory)
{
  auto allMem = sut.allocate(64);
  auto noFreeMem = sut.allocate(1);
  EXPECT_EQ(nullptr, noFreeMem.ptr);
  EXPECT_EQ(0, noFreeMem.length);
}

TEST_F(StackAllocatorTest, ThatAnIncreasingReallocationWithNoBlockInbetweenReturnsTheSameBuffer)
{
  auto mem = sut.allocate(4);
  *reinterpret_cast<int*>(mem.ptr) = 42;
  sut.reallocate(mem, 8);
  
  ASSERT_NE(nullptr, mem.ptr);
  EXPECT_EQ(42, *reinterpret_cast<int*>(mem.ptr));
  EXPECT_EQ(8, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatAnIncreasingReallocationWithABlockInbetweenReturnsADifferentBufferAndKeepsTheData)
{
  auto mem = sut.allocate(4);
  auto memInbetween = sut.allocate(4);

  auto memOiginalPtr = mem.ptr;
  *reinterpret_cast<int*>(mem.ptr) = 42;
  *reinterpret_cast<int*>(memInbetween.ptr) = 4711;
  sut.reallocate(mem, 8);

  ASSERT_NE(nullptr, mem.ptr);
  ASSERT_NE(memOiginalPtr, mem.ptr);
  EXPECT_EQ(42, *reinterpret_cast<int*>(mem.ptr));
  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(4711, *reinterpret_cast<int*>(memInbetween.ptr));

  deallocateAndCheckBlockIsThenEmpty(mem);
  deallocateAndCheckBlockIsThenEmpty(memInbetween);
}

TEST_F(StackAllocatorTest, ThatAReallocationFailsWhenOutOfMemory)
{
  auto mem = sut.allocate(64);
  auto originalMemPtr = mem.ptr;
  sut.reallocate(mem, 65);
  
  EXPECT_EQ(64, mem.length);
  EXPECT_EQ(originalMemPtr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatADecreasingReallocationWithOfTheLastAllocatedBlockKeepsTheMemPtr)
{
  auto mem = sut.allocate(16);
  auto originalMemPtr = mem.ptr;
  sut.reallocate(mem, 8);

  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(originalMemPtr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatADecreasingReallocationOfNotTheLastAllocatedBlockKeepsTheMemPtrAndTheData)
{
  auto mem = sut.allocate(16);
  auto memInBetween = sut.allocate(1);

  auto originalMemPtr = mem.ptr;
  *static_cast<int*>(mem.ptr) = 42;
  EXPECT_TRUE(sut.reallocate(mem, 8));

  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(originalMemPtr, mem.ptr);
  EXPECT_EQ(42, *static_cast<int*>(mem.ptr));

  deallocateAndCheckBlockIsThenEmpty(mem);
  deallocateAndCheckBlockIsThenEmpty(memInBetween);
}

TEST_F(StackAllocatorTest, ThatExpandingByZeroAnEmptyBlockResultsIntoAnEmptyBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_TRUE(sut.expand(mem, 0));
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatExpandingByZeroAFilledBlockResultsIntoTheSameBlock)
{
  auto mem = sut.allocate(1);
  auto oldPtr = mem.ptr;
  auto oldLength = mem.length;
  EXPECT_TRUE(sut.expand(mem, 0));
  EXPECT_EQ(oldPtr, mem.ptr);
  EXPECT_EQ(oldLength, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatExpandingBy16OfAnEmptyBlockResultsIntoANewBlock)
{
  auto mem = sut.allocate(0);

  EXPECT_TRUE(sut.expand(mem, 16));
  
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(16, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatExpandingByTheCapacityOfTheAllocatorOfAnEmptyBlockResultsIntoABlockWithAllocatorsCapacity)
{
  auto mem = sut.allocate(0);

  EXPECT_TRUE(sut.expand(mem, 64));

  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(64, mem.length);

  auto outOfMemoryBlock = sut.allocate(1);
  EXPECT_TRUE(!outOfMemoryBlock);
  EXPECT_FALSE(sut.expand(outOfMemoryBlock, 1));

  deallocateAndCheckBlockIsThenEmpty(mem);
}


TEST_F(StackAllocatorTest, ThatExpandingBeyondTheAllocatorsCapacityOfAnEmptyBlockResultsInFalse)
{
  auto mem = sut.allocate(0);

  EXPECT_FALSE(sut.expand(mem, 65));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(StackAllocatorTest, ThatExpandingBeyondTheAllocatorsCapacityAFilledBlockResultsInFalse)
{
  auto mem = sut.allocate(0);

  EXPECT_FALSE(sut.expand(mem, 65));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}


