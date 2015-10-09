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
#include <alb/freelist.hpp>
#include <alb/mallocator.hpp>
#include <alb/stack_allocator.hpp>
#include "TestHelpers/AllocatorBaseTest.h"
#include "TestHelpers/Base.h"

template <class T> class SharedListTest : public alb::test_helpers::AllocatorBaseTest<T> {
protected:
  void TearDown()
  {
    this->deallocateAndCheckBlockIsThenEmpty(this->mem);
  }
  alb::block mem;
};

using TypesForFreeListTest = ::testing::Types<alb::shared_freelist<alb::mallocator, 0, 16>,
                         alb::freelist<alb::mallocator, 0, 16>>;

TYPED_TEST_CASE(SharedListTest, TypesForFreeListTest);

TYPED_TEST(SharedListTest, ThatASimpleAllocationReturnsAtLeastTheRequiredSize)
{
  this->mem = this->sut.allocate(16);
  EXPECT_NE(nullptr, this->mem.ptr);
  EXPECT_EQ(16, this->mem.length);
}

TYPED_TEST(SharedListTest,
           ThatADeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  this->mem = this->sut.allocate(8);
  auto oldPtr = this->mem.ptr;
  this->deallocateAndCheckBlockIsThenEmpty(this->mem);

  this->mem = this->sut.allocate(8);
  EXPECT_EQ(oldPtr, this->mem.ptr);
}

TYPED_TEST(SharedListTest,
           ThatSeveralDeallocatedMemBlockGetsReusedWhenNewAllocatedWithTheSameAndDifferentSize)
{
  auto mem1 = this->sut.allocate(8);
  auto mem2 = this->sut.allocate(8);
  auto oldPtr1 = mem1.ptr;
  auto oldPtr2 = mem2.ptr;

  this->deallocateAndCheckBlockIsThenEmpty(mem1);
  this->deallocateAndCheckBlockIsThenEmpty(mem2);

  mem1 = this->sut.allocate(12);
  mem2 = this->sut.allocate(12);

  EXPECT_EQ(oldPtr2, mem1.ptr);
  EXPECT_EQ(oldPtr1, mem2.ptr);

  this->deallocateAndCheckBlockIsThenEmpty(mem1);
  this->deallocateAndCheckBlockIsThenEmpty(mem2);
}

TYPED_TEST(SharedListTest, ThatReallocatingAnEmptyBlockResultsToBlockOfBoundsSize)
{
  EXPECT_TRUE(this->sut.reallocate(this->mem, 8));
  EXPECT_EQ(16, this->mem.length);
}

TYPED_TEST(SharedListTest, ThatReallocatingAFilledBlockToNonZeroIsRejected)
{
  this->mem = this->sut.allocate(8);
  EXPECT_FALSE(this->sut.reallocate(this->mem, 2));
  EXPECT_EQ(16, this->mem.length);
}

TYPED_TEST(SharedListTest, ThatBlocksOutsideTheBoundsAreNotRecognizedAsOwned)
{
  EXPECT_FALSE(this->sut.owns(alb::block()));
  EXPECT_FALSE(this->sut.owns(alb::block(nullptr, 64)));

  alb::shared_freelist<alb::mallocator, 16, 32> sutNotStartingAtZero;
  EXPECT_FALSE(sutNotStartingAtZero.owns(alb::block(nullptr, 15)));
  EXPECT_FALSE(sutNotStartingAtZero.owns(alb::block(nullptr, 33)));
}

TYPED_TEST(SharedListTest, ThatAProvidedBlockIsRecongizedAOwned)
{
  this->mem = this->sut.allocate(16);
  EXPECT_TRUE(this->sut.owns(this->mem));
}

template <class T> class FreeListWithParametrizedTest : public ::testing::Test {
protected:
  FreeListWithParametrizedTest()
    : sut(16, 42)
  {
  }
  T sut;
};

using TypesForFreeListWithParametrizedTest = ::testing::Types<alb::shared_freelist<alb::mallocator, alb::internal::DynasticDynamicSet,
                                              alb::internal::DynasticDynamicSet>,
                         alb::freelist<alb::mallocator, alb::internal::DynasticDynamicSet,
                                       alb::internal::DynasticDynamicSet>>;

TYPED_TEST_CASE(FreeListWithParametrizedTest, TypesForFreeListWithParametrizedTest);

TYPED_TEST(FreeListWithParametrizedTest, ThatUpperAndLowerBoundIsSet)
{
  EXPECT_EQ(16, this->sut.min_size());
  EXPECT_EQ(42, this->sut.max_size());
}

TYPED_TEST(FreeListWithParametrizedTest, ThatAllocationsBeyondTheBoundariesAreRejected)
{
  size_t rejectedValues[] = {0, 4, 15, 43, 64};
  for (auto i : rejectedValues) {
    auto mem = this->sut.allocate(i);
    EXPECT_FALSE(mem);
  }
}

TEST(FreeListWithTruncatedDeallocationParentAllocatorTest, ThatThePreAllocatedBlocksAreInLine)
{
  alb::shared_freelist<alb::stack_allocator<1024>, 0, 16, 1024, 4> sut;
  alb::block mem[8]; // 0 3 2 1  0 3 2 1 order of allocations
  mem[0] = sut.allocate(16);
  for (size_t i = 3; i > 0; i--) {
    mem[i] = sut.allocate(16);
  }
  mem[4] = sut.allocate(16);
  for (size_t i = 7; i > 4; i--) {
    mem[i] = sut.allocate(16);
  } // 0 1 2 3 0 1 2 3
  for (size_t i = 0; i < 7; i++) {
    EXPECT_EQ(static_cast<char *>(mem[i].ptr) + 16, mem[i + 1].ptr) << "Failure at " << i;
  }
}
