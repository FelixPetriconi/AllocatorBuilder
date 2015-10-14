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
#include <alb/bucketizer.hpp>
#include <alb/freelist.hpp>
#include <alb/mallocator.hpp>
#include "TestHelpers/AllocatorBaseTest.h"
#include "TestHelpers/Data.h"
#include "TestHelpers/Base.h"

using namespace alb::test_helpers;

using AllocatorUnderTest = alb::bucketizer<alb::shared_freelist<alb::mallocator, alb::internal::DynasticDynamicSet,
                                             alb::internal::DynasticDynamicSet>,
                        17, 64, 16> ;

class BucketizerTest : public AllocatorBaseTest<AllocatorUnderTest> {
};

TEST_F(BucketizerTest, ThatMinAndMaxSizeOfTheBucketItemsAreSetSpecifiedByTheParameter)
{
  ASSERT_EQ(3u, AllocatorUnderTest::number_of_buckets);

  EXPECT_EQ(17u, sut._buckets[0].min_size());
  EXPECT_EQ(32u, sut._buckets[0].max_size());

  EXPECT_EQ(33u, sut._buckets[1].min_size());
  EXPECT_EQ(48u, sut._buckets[1].max_size());

  EXPECT_EQ(49u, sut._buckets[2].min_size());
  EXPECT_EQ(64u, sut._buckets[2].max_size());
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
  alb::block dummy;
  sut.deallocate(dummy);
}

TEST_F(BucketizerTest,
       ThatAllocatingAtTheLowerEdgeOfABucketResultsInABlockWithTheUpperEdgeOfThatAllocator)
{
  for (size_t i = 0; i < AllocatorUnderTest::number_of_buckets; i++) {
    auto mem = sut.allocate(17 + i * 16);
    EXPECT_EQ(16u + (i + 1) * 16, mem.length);
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
}

TEST_F(BucketizerTest,
       ThatAllocatingAtTheUpperEdgeOfABucketResultsInABlockWithTheUpperEdgeOfThatAllocator)
{
  for (size_t i = 0; i < AllocatorUnderTest::number_of_buckets; i++) {
    auto mem = sut.allocate((i + 2) * 16);
    EXPECT_EQ((i + 2) * 16, mem.length);
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
}

TEST_F(BucketizerTest, ThatAReallocationWithInTheEdgesOfABucketItemTheLengthAndTheContentIsTheSame)
{
  auto mem = sut.allocate(32);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  auto originalPtr = mem.ptr;
  EXPECT_TRUE(sut.reallocate(mem, 20));

  EXPECT_EQ(32u, mem.length);
  EXPECT_EQ(originalPtr, mem.ptr);

  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 32);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest,
       ThatAReallocationBeyondTheUpperEdgeOfABucketItemCrossesTheBucketItemAndPreservesTheContent)
{
  auto mem = sut.allocate(32);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  auto originalPtr = mem.ptr;
  EXPECT_TRUE(sut.reallocate(mem, 33));

  EXPECT_EQ(48u, mem.length);
  EXPECT_NE(originalPtr, mem.ptr);

  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 32);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatAReallocationOfAnEmptyBlockOutsideTheBoundsBytesReturnsFalse)
{
  alb::block emptyBlock;
  EXPECT_FALSE(sut.reallocate(emptyBlock, 1));
  EXPECT_FALSE(sut.reallocate(emptyBlock, 65));
}

TEST_F(BucketizerTest, ThatAReallocationOfAnEmptyBlockWithinTheBoundsBytesReturnsAValidBlock)
{
  alb::block mem;
  EXPECT_TRUE(sut.reallocate(mem, 32));
  EXPECT_EQ(32u, mem.length);
  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatAReallocationOfAFilledBlockToZeroDeallocatesTheMemory)
{
  auto mem = sut.allocate(32);
  EXPECT_TRUE(sut.reallocate(mem, 0));
  EXPECT_FALSE(mem);
}

TEST_F(
    BucketizerTest,
    ThatAReallocationBeyondTheLowerEdgeOfABucketItemCrossesTheBucketItemAndPreservesTheStrippedContent)
{
  auto mem = sut.allocate(48);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  auto originalPtr = mem.ptr;
  EXPECT_TRUE(sut.reallocate(mem, 20));

  EXPECT_EQ(32u, mem.length);
  EXPECT_NE(originalPtr, mem.ptr);

  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 32);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(BucketizerTest, ThatReturnsFalseIfTheBlockIsOutOgBoundsOrInvalid)
{
  EXPECT_FALSE(sut.owns(alb::block()));
  EXPECT_FALSE(sut.owns(alb::block(nullptr, 1)));
  EXPECT_FALSE(sut.owns(alb::block(nullptr, 65)));
}
