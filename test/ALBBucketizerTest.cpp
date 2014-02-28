///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
//////////////////////////////////////////////////////////////////
#pragma once

#include <gtest/gtest.h>
#include "ALBTestHelpers.h"
#include "ALBBucketizer.h"
#include "ALBFreeList.h"
#include "ALBMallocator.h"
#include "ALBTestHelpersAllocatorBaseTest.h"
#include "ALBTestHelpersData.h"

using namespace ALB::TestHelpers;

typedef ALB::Bucketizer<
  ALB::SharedFreeList<
    ALB::Mallocator, ALB::DynamicSetSize, ALB::DynamicSetSize>,
    17, 64, 16> AllocatorUnderTest;

class BucketizerTest : public AllocatorBaseTest<AllocatorUnderTest> {
};

TEST_F(BucketizerTest, ThatMinAndMaxSizeOfTheBucketItemsAreSetSpecifiedByTheParameter)
{
  ASSERT_EQ(3, AllocatorUnderTest::number_of_buckets);

  EXPECT_EQ(17, sut._buckets[0].min_size());
  EXPECT_EQ(32, sut._buckets[0].max_size());

  EXPECT_EQ(33, sut._buckets[1].min_size());
  EXPECT_EQ(48, sut._buckets[1].max_size());

  EXPECT_EQ(49, sut._buckets[2].min_size());
  EXPECT_EQ(64, sut._buckets[2].max_size());
}

TEST_F(BucketizerTest, ThatAllocatingBeyondTheAllocatorsRangeResultsInAnEmptyBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_FALSE(mem);

  mem = sut.allocate(4);
  EXPECT_FALSE(mem);

  mem = sut.allocate(65);
  EXPECT_FALSE(mem);

  mem = sut.allocate(100);
  EXPECT_FALSE(mem);

  // Just to satisfy the code coverage
  sut.deallocate(ALB::Block());
}

TEST_F(BucketizerTest, ThatAllocatingAtTheLowerEdgeOfABucketResultsInABlockWithTheUpperEdgeOfThatAllocator)
{
  for (size_t i = 0; i < AllocatorUnderTest::number_of_buckets; i++) {
    auto mem = sut.allocate(17 + i*16);
    EXPECT_EQ(16 + (i+1)*16, mem.length);
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
}

TEST_F(BucketizerTest, ThatAllocatingAtTheUpperEdgeOfABucketResultsInABlockWithTheUpperEdgeOfThatAllocator)
{
  for (size_t i = 0; i < AllocatorUnderTest::number_of_buckets; i++) {
    auto mem = sut.allocate((i+2)*16);
    EXPECT_EQ((i+2)*16, mem.length);
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
}

TEST_F(BucketizerTest, ThatAReallocationWithInTheEdgesOfABucketItemTheLengthAndTheContentIsTheSame)
{
  auto mem = sut.allocate(32);
  ALB::TestHelpers::fillBlockWithReferenceData<int>(mem);

  auto originalPtr = mem.ptr;
  EXPECT_TRUE(sut.reallocate(mem, 20));

  EXPECT_EQ(32, mem.length);
  EXPECT_EQ(originalPtr, mem.ptr);

  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), 32);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatAReallocationBeyondTheUpperEdgeOfABucketItemCrossesTheBucketItemAndPreservesTheContent)
{
  auto mem = sut.allocate(32);
  ALB::TestHelpers::fillBlockWithReferenceData<int>(mem);

  auto originalPtr = mem.ptr;
  EXPECT_TRUE(sut.reallocate(mem, 33));

  EXPECT_EQ(48, mem.length);
  EXPECT_NE(originalPtr, mem.ptr);

  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), 32);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatAReallocationOfAnEmptyBlockOutsideTheBoundsBytesReturnsFalse)
{
  EXPECT_FALSE(sut.reallocate(ALB::Block(), 1));
  EXPECT_FALSE(sut.reallocate(ALB::Block(), 65));
}

TEST_F(BucketizerTest, ThatAReallocationOfAnEmptyBlockWithinTheBoundsBytesReturnsAValidBlock)
{
  ALB::Block mem;
  EXPECT_TRUE(sut.reallocate(mem, 32));
  EXPECT_EQ(32, mem.length);
  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatAReallocationOfAFilledBlockToZeroDeallocatesTheMemory)
{
  auto mem = sut.allocate(32);
  EXPECT_TRUE(sut.reallocate(mem, 0));
  EXPECT_FALSE(mem);
}


TEST_F(BucketizerTest, ThatAReallocationBeyondTheLowerEdgeOfABucketItemCrossesTheBucketItemAndPreservesTheStrippedContent)
{
  auto mem = sut.allocate(48);
  ALB::TestHelpers::fillBlockWithReferenceData<int>(mem);

  auto originalPtr = mem.ptr;
  EXPECT_TRUE(sut.reallocate(mem, 20));

  EXPECT_EQ(32, mem.length);
  EXPECT_NE(originalPtr, mem.ptr);

  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), 32);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatReturnsFalseIfTheBlockIsOutOgBoundsOrInvalid)
{
  EXPECT_FALSE(sut.owns(ALB::Block()));
  EXPECT_FALSE(sut.owns(ALB::Block(nullptr, 1)));
  EXPECT_FALSE(sut.owns(ALB::Block(nullptr, 65)));
}