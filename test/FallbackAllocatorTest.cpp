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
#include <alb/fallback_allocator.hpp>
#include <alb/stack_allocator.hpp>
#include <alb/shared_heap.hpp>
#include <alb/mallocator.hpp>
#include "TestHelpers/AllocatorBaseTest.h"
#include "TestHelpers/Data.h"
#include "TestHelpers/Base.h"

using namespace alb::test_helpers;

class FallbackAllocatorTest
    : public AllocatorBaseTest<alb::fallback_allocator<alb::stack_allocator<32,4>, alb::mallocator>> {
protected:
  void SetUp()
  {
    mem = sut.allocate(1);
    StartPtrPrimary = mem.ptr;
    deallocateAndCheckBlockIsThenEmpty(mem);
  }

  void TearDown()
  {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }

  void *StartPtrPrimary;
  alb::block mem;
};

TEST_F(FallbackAllocatorTest, ThatAllocatingZeroBytesReturnsAnEmptyBlock)
{
  mem = sut.allocate(0);
  EXPECT_FALSE(mem);
}

TEST_F(FallbackAllocatorTest, ThatAllocatingUpToTheLimitsOfThePrimaryIsDoneByThePrimary)
{
  mem = sut.allocate(32);
  EXPECT_EQ(32u, mem.length);
  EXPECT_EQ(StartPtrPrimary, mem.ptr);
}

TEST_F(FallbackAllocatorTest, ThatAllocatingBeyondTheLimitsOfThePrimaryIsDoneByTheFallback)
{
  mem = sut.allocate(33);
  EXPECT_GE(33u, mem.length);
  EXPECT_NE(StartPtrPrimary, mem.ptr);
}

TEST_F(FallbackAllocatorTest,
       ThatIncreasingReallocatingWithinTheLimitsOfThePrimaryIsHandledByThePrimary)
{
  mem = sut.allocate(8);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 16));
  EXPECT_EQ(16u, mem.length);
  EXPECT_EQ(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 8);
}

TEST_F(FallbackAllocatorTest,
       ThatIncreasingReallocatingOfABlockOwnedByTheFallbackStaysAtTheFallback)
{
  mem = sut.allocate(64);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 128));
  EXPECT_EQ(128u, mem.length);
  EXPECT_NE(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 64);
}

TEST_F(FallbackAllocatorTest, ThatDecreasingReallocatingOfAPrimaryOwnedBlockIsHandledByThePrimary)
{
  mem = sut.allocate(16);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 8));
  EXPECT_EQ(8u, mem.length);
  EXPECT_EQ(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 8);
}

TEST_F(FallbackAllocatorTest,
       ThatIncreasingReallocatingBeyondTheLimitsOfThePrimaryIsHandledByTheFallback)
{
  mem = sut.allocate(8);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 64));
  EXPECT_EQ(64u, mem.length);
  EXPECT_NE(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 8);
}

TEST_F(FallbackAllocatorTest, ThatDecreasingReallocatingOfAFallbackOwnedBlockIsHandledByTheFallback)
{
  mem = sut.allocate(64);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, 16));
  EXPECT_EQ(16u, mem.length);
  EXPECT_NE(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 16);
}

TEST_F(FallbackAllocatorTest,
       ThatExpandingOfABlockOwnedByThePrimaryWithinTheLimitsOfThePrimaryIsDone)
{
  mem = sut.allocate(16);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, 8));
  EXPECT_EQ(24u, mem.length);
  EXPECT_EQ(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 8);
}

TEST_F(FallbackAllocatorTest,
       ThatExpandingOfABlockOwnedByThePrimaryBeyondTheLimitsOfThePrimaryIsRejected)
{
  mem = sut.allocate(16);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_FALSE(sut.expand(mem, 64));
  EXPECT_EQ(16u, mem.length);
  EXPECT_EQ(StartPtrPrimary, mem.ptr);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 16);
}

TEST_F(FallbackAllocatorTest,
       ThatExpandingOfABlockOwnedByTheFallbackIsRejectedBecauseItDoesNotSupportExpand)
{
  mem = sut.allocate(64);
  alb::test_helpers::fillBlockWithReferenceData<int>(mem);

  EXPECT_FALSE(sut.expand(mem, 64));
  EXPECT_EQ(64u, mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), 64);
}

TEST(FallbackAllocatorWithPrimaryAndFallbackImplemntsOwnsTest, ThatOwnsIsProcessedCorrectly)
{
  alb::fallback_allocator<alb::stack_allocator<32, 4>, alb::shared_heap<alb::mallocator, 64, 4>>
      sut;

  auto memFromPrimary = sut.allocate(32);
  auto memFromFallback = sut.allocate(64);

  EXPECT_TRUE(sut.owns(memFromPrimary));
  EXPECT_TRUE(sut.owns(memFromFallback));

  EXPECT_FALSE(sut.owns(alb::block()));
}

TEST(FallbackAllocatorWithPrimaryAndFallbackImplemntsExpandTest, ThatExpandIsProcessedCorrectly)
{
  alb::fallback_allocator<alb::stack_allocator<32, 4>, alb::shared_heap<alb::mallocator, 64, 4>>
      sut;

  auto memFromPrimary = sut.allocate(16);
  auto memFromFallback = sut.allocate(32);

  EXPECT_TRUE(sut.expand(memFromPrimary, 16));
  EXPECT_TRUE(sut.expand(memFromFallback, 16));
}

TEST(FallbackAllocatorWithOnlyPrimaryImplementsExpandTest, ThatExpandIsProcessedCorrectly)
{
  alb::fallback_allocator<alb::stack_allocator<32, 4>, alb::mallocator> sut;

  auto memFromPrimary = sut.allocate(16);
  auto memFromFallback = sut.allocate(32);

  EXPECT_TRUE(sut.expand(memFromPrimary, 16));
  EXPECT_FALSE(sut.expand(memFromFallback, 16));
}

TEST(FallbackAllocatorWithOnlyPrimaryImplementsExpandTest, ThatExpandFailsIfPrimaryIsOutOfMemory)
{
  alb::fallback_allocator<alb::stack_allocator<32, 4>, alb::mallocator> sut;

  auto memFromPrimary = sut.allocate(32);

  EXPECT_FALSE(sut.expand(memFromPrimary, 16));
}
