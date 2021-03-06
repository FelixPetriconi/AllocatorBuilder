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
#include <alb/stack_allocator.hpp>

#include "util.hpp"

#include "TestHelpers/Base.h"
#include "TestHelpers/AllocatorBaseTest.h"


class stack_allocatorTest
    : public alb::test_helpers::AllocatorBaseTest<alb::stack_allocator<64, 4>> {
};

TEST_F(stack_allocatorTest, ThatAllocatingZeroBytesReturnsAnEmptyMemoryBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatAllocatingOneBytesReturnsABlockOfOneByte)
{
  auto mem = sut.allocate(1);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(4u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatAllocatingTheCompleteStackSizeIsPossible)
{
  auto mem = sut.allocate(64);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(64u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatAllocatingTwoMemoryBlocksUsesContiguousMemory)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);
  EXPECT_EQ(mem2.ptr, static_cast<char *>(mem1.ptr) + 8);
  EXPECT_EQ(8u, mem1.length);
  EXPECT_EQ(8u, mem2.length);

  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem1);
}

TEST_F(stack_allocatorTest, ThatAFreedBlockWhichWasTheLastAllocatedOnesGetsReusedForANewAllocation)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);

  ignore_unused(mem1);

  auto ptrOf2ndLocation = mem2.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem2);

  auto mem3 = sut.allocate(8);
  EXPECT_EQ(ptrOf2ndLocation, mem3.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem3);
}

TEST_F(stack_allocatorTest,
       ThatAFreedBlockWhichWasNotTheLastAllocatedOnesGetsNotReusedForANewAllocation)
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

TEST_F(stack_allocatorTest, ThatANullBlockIsReturnedWhenTheAllocatorIsOutOfMemory)
{
  auto allMem = sut.allocate(64);
  auto noFreeMem = sut.allocate(1);

  ignore_unused(allMem);

  EXPECT_EQ(nullptr, noFreeMem.ptr);
  EXPECT_EQ(0u, noFreeMem.length);
}

TEST_F(stack_allocatorTest, ThatAnIncreasingReallocationWithNoBlockInbetweenReturnsTheSameBuffer)
{
  auto mem = sut.allocate(4);
  *static_cast<int*>(mem.ptr) = 42;
  sut.reallocate(mem, 8);

  ASSERT_NE(nullptr, mem.ptr);
  EXPECT_EQ(42, *static_cast<int*>(mem.ptr));
  EXPECT_EQ(8u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest,
       ThatAnIncreasingReallocationWithABlockInbetweenReturnsADifferentBufferAndKeepsTheData)
{
  auto mem = sut.allocate(4);
  auto memInbetween = sut.allocate(4);

  auto memOiginalPtr = mem.ptr;
  *static_cast<int *>(mem.ptr) = 42;
  *static_cast<int *>(memInbetween.ptr) = 4711;
  sut.reallocate(mem, 8);

  ASSERT_NE(nullptr, mem.ptr);
  ASSERT_NE(memOiginalPtr, mem.ptr);
  EXPECT_EQ(42, *static_cast<int *>(mem.ptr));
  EXPECT_EQ(8u, mem.length);
  EXPECT_EQ(4711, *static_cast<int *>(memInbetween.ptr));

  deallocateAndCheckBlockIsThenEmpty(mem);
  deallocateAndCheckBlockIsThenEmpty(memInbetween);
}

TEST_F(stack_allocatorTest, ThatAReallocationFailsWhenOutOfMemory)
{
  auto mem = sut.allocate(64);
  auto originalMemPtr = mem.ptr;
  sut.reallocate(mem, 65);

  EXPECT_EQ(64u, mem.length);
  EXPECT_EQ(originalMemPtr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatADecreasingReallocationWithOfTheLastAllocatedBlockKeepsTheMemPtr)
{
  auto mem = sut.allocate(16);
  auto originalMemPtr = mem.ptr;
  sut.reallocate(mem, 8);

  EXPECT_EQ(8u, mem.length);
  EXPECT_EQ(originalMemPtr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest,
       ThatADecreasingReallocationOfNotTheLastAllocatedBlockKeepsTheMemPtrAndTheData)
{
  auto mem = sut.allocate(16);
  auto memInBetween = sut.allocate(1);

  auto originalMemPtr = mem.ptr;
  *static_cast<int *>(mem.ptr) = 42;
  EXPECT_TRUE(sut.reallocate(mem, 8));

  EXPECT_EQ(8u, mem.length);
  EXPECT_EQ(originalMemPtr, mem.ptr);
  EXPECT_EQ(42, *static_cast<int *>(mem.ptr));

  deallocateAndCheckBlockIsThenEmpty(mem);
  deallocateAndCheckBlockIsThenEmpty(memInBetween);
}

TEST_F(stack_allocatorTest, ThatExpandingByZeroAnEmptyBlockResultsIntoAnEmptyBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_TRUE(sut.expand(mem, 0));
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatExpandingByZeroAFilledBlockResultsIntoTheSameBlock)
{
  auto mem = sut.allocate(1);
  auto oldPtr = mem.ptr;
  auto oldLength = mem.length;
  EXPECT_TRUE(sut.expand(mem, 0));
  EXPECT_EQ(oldPtr, mem.ptr);
  EXPECT_EQ(oldLength, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatExpandingBy16OfAnEmptyBlockResultsIntoANewBlock)
{
  auto mem = sut.allocate(0);

  EXPECT_TRUE(sut.expand(mem, 16));

  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(16u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(
    stack_allocatorTest,
    ThatExpandingByTheCapacityOfTheAllocatorOfAnEmptyBlockResultsIntoABlockWithAllocatorsCapacity)
{
  auto mem = sut.allocate(0);

  EXPECT_TRUE(sut.expand(mem, 64));

  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(64u, mem.length);

  auto outOfMemoryBlock = sut.allocate(1);
  EXPECT_TRUE(!outOfMemoryBlock);
  EXPECT_FALSE(sut.expand(outOfMemoryBlock, 1));

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatExpandingBeyondTheAllocatorsCapacityOfAnEmptyBlockResultsInFalse)
{
  auto mem = sut.allocate(0);

  EXPECT_FALSE(sut.expand(mem, 65));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(stack_allocatorTest, ThatExpandingBeyondTheAllocatorsCapacityAFilledBlockResultsInFalse)
{
  auto mem = sut.allocate(0);

  EXPECT_FALSE(sut.expand(mem, 65));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}
