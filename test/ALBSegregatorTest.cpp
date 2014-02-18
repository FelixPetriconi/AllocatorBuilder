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
#include "ALBSegregator.h"
#include "ALBTestHelpers.h"
#include "ALBStackAllocator.h"
#include "ALBSharedHeap.h"
#include "ALBMallocator.h"

namespace
{
  const size_t LargeBlockSize = 64;
}

class SegregatorTest : public 
  ALB::TestHelpers::AllocatorBaseTest<
    ALB::Segregator<ALB::StackAllocator<32>::max_size+1, ALB::StackAllocator<32>, 
    ALB::SharedHeap<ALB::Mallocator, 512,4>>>
{
protected:
  void SetUp() {
    mem = sut.allocate(4);
    StartSmallAllocatorPtr = mem.ptr;
    deallocateAndCheckBlockIsThenEmpty(mem);

    mem = sut.allocate(ALB::StackAllocator<32>::max_size + 1);
    StartLargeAllocatorPtr = mem.ptr;
    deallocateAndCheckBlockIsThenEmpty(mem);

    ASSERT_NE(StartSmallAllocatorPtr, StartLargeAllocatorPtr);
  }

  void TearDown() {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }

  void* StartSmallAllocatorPtr;
  void* StartLargeAllocatorPtr;
  ALB::Block mem;
};

TEST_F(SegregatorTest, ThatAllocatingZeroBytesResultsInAnEmptyBlock)
{
  mem = sut.allocate(0);

  EXPECT_EQ(0, mem.length);
  EXPECT_EQ(nullptr, mem.ptr);
}

TEST_F(SegregatorTest, ThatAllocating8BytesResultsInABlockOfThatSize)
{
  mem = sut.allocate(8);

  EXPECT_EQ(8, mem.length);
  EXPECT_NE(nullptr, mem.ptr);
}

TEST_F(SegregatorTest, ThatAllocatingSmallAllocatorsSizeReturnsABlockOfThatSize)
{
  mem = sut.allocate(ALB::StackAllocator<32>::max_size);

  EXPECT_EQ(ALB::StackAllocator<32>::max_size, mem.length);
  EXPECT_NE(nullptr, mem.ptr);
}

TEST_F(SegregatorTest, ThatAllocatingLargeAllocatorsSizeReturnsABlockOfThatSizeAndItsBufferDoesNotComeFromTheSmallAllocator)
{
  mem = sut.allocate(LargeBlockSize);

  EXPECT_EQ(LargeBlockSize, mem.length);
  EXPECT_EQ(StartLargeAllocatorPtr, mem.ptr);
}


TEST_F(SegregatorTest, ThatReallocatingASmallBlockWithInTheBoundsOfTheSmallAllocatorStaysThere)
{
  mem = sut.allocate(8);
  ALB::TestHelpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, ALB::StackAllocator<32>::max_size));
  ALB::TestHelpers::EXPECT_MEM_EQ(mem.ptr, (void*)ALB::TestHelpers::ReferenceData.data(), 8);

  EXPECT_EQ(ALB::StackAllocator<32>::max_size, mem.length);
  EXPECT_EQ(StartSmallAllocatorPtr, mem.ptr);
}

TEST_F(SegregatorTest, ThatReallocatingASmallBlockOutOfTheBoundsOfTheSmallAllocatorItGoesToTheLargeAllocator)
{
  mem = sut.allocate(8);
  ALB::TestHelpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, ALB::StackAllocator<32>::max_size+1));
  ALB::TestHelpers::EXPECT_MEM_EQ(mem.ptr, (void*)ALB::TestHelpers::ReferenceData.data(), 8);

  EXPECT_LE(ALB::StackAllocator<32>::max_size, mem.length);
  EXPECT_EQ(StartLargeAllocatorPtr, mem.ptr);
}

// TODO
// Does not work so far, because the insito reallocation of the SharedHeap is not implemented so far
TEST_F(SegregatorTest, ThatReallocatingALargeBlockToASmallerSizeStaysAtTheLargeAllocator) 
{
  mem = sut.allocate(LargeBlockSize);
  ALB::TestHelpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 4));
  ALB::TestHelpers::EXPECT_MEM_EQ(mem.ptr, (void*)ALB::TestHelpers::ReferenceData.data(), 4);

  EXPECT_EQ(4, mem.length);
  EXPECT_EQ(StartLargeAllocatorPtr, mem.ptr);
}