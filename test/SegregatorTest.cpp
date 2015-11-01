///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>
#include <alb/segregator.hpp>
#include <alb/stack_allocator.hpp>
#include <alb/shared_heap.hpp>
#include <alb/mallocator.hpp>
#include "TestHelpers/AllocatorBaseTest.h"
#include "TestHelpers/Data.h"
#include "TestHelpers/Base.h"

namespace {
  const size_t LargeBlockSize = 64;
}

class SegregatorTest
    : public alb::test_helpers::AllocatorBaseTest<
          alb::segregator<alb::stack_allocator<32>::max_size, alb::stack_allocator<32, 4>,
                          alb::shared_heap<alb::mallocator, 512, 4>>> {
protected:
  void SetUp()
  {
    mem = sut.allocate(4);
    StartSmallAllocatorPtr = mem.ptr;
    deallocateAndCheckBlockIsThenEmpty(mem);

    mem = sut.allocate(alb::stack_allocator<32>::max_size + 1);
    StartLargeAllocatorPtr = mem.ptr;
    deallocateAndCheckBlockIsThenEmpty(mem);

    ASSERT_NE(StartSmallAllocatorPtr, StartLargeAllocatorPtr);
  }

  void TearDown()
  {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }

  void *StartSmallAllocatorPtr;
  void *StartLargeAllocatorPtr;
  alb::block mem;
};

TEST_F(SegregatorTest, ThatAllocatingZeroBytesResultsInAnEmptyBlock)
{
  mem = sut.allocate(0);

  EXPECT_EQ(0u, mem.length);
  EXPECT_EQ(nullptr, mem.ptr);
}

TEST_F(SegregatorTest, ThatAllocating8BytesResultsInABlockOfThatSize)
{
  mem = sut.allocate(8);

  EXPECT_EQ(8u, mem.length);
  EXPECT_NE(nullptr, mem.ptr);
}

TEST_F(SegregatorTest, ThatAllocatingSmallAllocatorsSizeReturnsABlockOfThatSize)
{
  mem = sut.allocate(alb::stack_allocator<32>::max_size);

  EXPECT_EQ(alb::stack_allocator<32>::max_size, mem.length);
  EXPECT_NE(nullptr, mem.ptr);
}

TEST_F(
    SegregatorTest,
    ThatAllocatingLargeAllocatorsSizeReturnsABlockOfThatSizeAndItsBufferDoesNotComeFromTheSmallAllocator)
{
  mem = sut.allocate(LargeBlockSize);

  EXPECT_EQ(LargeBlockSize, mem.length);
  EXPECT_EQ(StartLargeAllocatorPtr, mem.ptr);
}

TEST_F(SegregatorTest, ThatReallocatingASmallBlockWithInTheBoundsOfTheSmallAllocatorStaysThere)
{
  mem = sut.allocate(8);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, alb::stack_allocator<32>::max_size));
  alb::test_helpers::EXPECT_MEM_EQ(mem.ptr, (void *)alb::test_helpers::ReferenceData.data(), 8);

  EXPECT_EQ(alb::stack_allocator<32>::max_size, mem.length);
  EXPECT_EQ(StartSmallAllocatorPtr, mem.ptr);
}

TEST_F(SegregatorTest,
       ThatReallocatingASmallBlockOutOfTheBoundsOfTheSmallAllocatorItGoesToTheLargeAllocator)
{
  mem = sut.allocate(8);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, alb::stack_allocator<32>::max_size + 1));
  alb::test_helpers::EXPECT_MEM_EQ(mem.ptr, (void *)alb::test_helpers::ReferenceData.data(), 8);

  EXPECT_LE(alb::stack_allocator<32>::max_size, mem.length);
  EXPECT_EQ(StartLargeAllocatorPtr, mem.ptr);
}

TEST_F(SegregatorTest, ThatReallocatingALargeBlockToASmallerSizeGoesToTheSmallAllocator)
{
  mem = sut.allocate(LargeBlockSize);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 4));
  alb::test_helpers::EXPECT_MEM_EQ(mem.ptr, (void *)alb::test_helpers::ReferenceData.data(), 4);

  EXPECT_EQ(4u, mem.length);
  EXPECT_EQ(StartSmallAllocatorPtr, mem.ptr);
}
