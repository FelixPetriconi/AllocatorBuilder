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
#include <alb/affix_allocator.hpp>
#include <alb/stack_allocator.hpp>
#include <alb/memory_corruption_detector.hpp>
#include "ALBTestHelpersAllocatorBaseTest.h"
#include "ALBTestHelpers.h"

namespace
{
  const unsigned PrefixMarker = 0xbaadf00d;
  const uint64_t LargePrefixMarker = 0xfefefefebaadf00d;
  const unsigned SufixMarker = 0xf000baaa;
}

template <class T>
class affix_allocatorTest : public alb::test_helpers::AllocatorBaseTest<T> 
{
protected:
  void TearDown() {
    this->deallocateAndCheckBlockIsThenEmpty(this->mem);
  }

  void checkAffixContent() {
    if (T::prefix_size > 0) {
      EXPECT_EQ(T::prefix::pattern, *(static_cast<typename T::prefix::value_type*>(this->mem.ptr) - 1) ) 
        << "Problem with type " << typeid(T).name();
    }
    if (T::sufix_size > 0) {
      EXPECT_EQ(T::sufix::pattern, *(static_cast<typename T::sufix::value_type*>(this->mem.ptr) + this->mem.length / sizeof(typename T::sufix::value_type)) ) 
        << "Problem with type " << typeid(T).name();
    }
  }
  alb::Block mem;
};

typedef ::testing::Types<
  alb::affix_allocator<
    alb::stack_allocator<512,4>, 
      alb::memory_corruption_detector<unsigned, PrefixMarker>
  >,
  alb::affix_allocator<
    alb::stack_allocator<512,4>, 
      alb::affix_allocator_helper::no_affix,
      alb::memory_corruption_detector<unsigned, SufixMarker>
  >,
  alb::affix_allocator<
    alb::stack_allocator<512, 4>, 
      alb::memory_corruption_detector<unsigned, PrefixMarker>,
      alb::memory_corruption_detector<unsigned, SufixMarker>
  >,
  alb::affix_allocator<
    alb::stack_allocator<512, 4>, 
      alb::memory_corruption_detector<uint64_t, LargePrefixMarker>,
        alb::memory_corruption_detector<unsigned, SufixMarker>
  >
> TypesToTest;

TYPED_TEST_CASE(affix_allocatorTest, TypesToTest);


TYPED_TEST(affix_allocatorTest, ThatAnEmptyAllocationReturnsAnEmptyBlock)
{
  this->mem = this->sut.allocate(0);

  EXPECT_EQ(0, this->mem.length);
  EXPECT_EQ(nullptr, this->mem.ptr);
}

TYPED_TEST(affix_allocatorTest, ThatAFullAllocationReturnsAnEmptyBlockBecauseOfMissingSpaceForTheAffixes)
{
  this->mem = this->sut.allocate(512);

  EXPECT_EQ(0, this->mem.length);
  EXPECT_EQ(nullptr, this->mem.ptr);
}

TYPED_TEST(affix_allocatorTest, ThatASmallAllocationsReturnsTheRequestedSizeAndThatTheMarkerAreSetCorrectly)
{
  this->mem = this->sut.allocate(8);
  
  EXPECT_EQ(8, this->mem.length);

  this->checkAffixContent();
}

TYPED_TEST(affix_allocatorTest, ThatASmallAllocatedBlockIncreasingReallocatedIntoANewMemAreaKeepsTheOldMarker)
{
  this->mem = this->sut.allocate(8);
  auto memInBetween = this->sut.allocate(4);

  EXPECT_TRUE(this->sut.reallocate(this->mem, 16));
  
  EXPECT_EQ(16, this->mem.length);
  this->checkAffixContent();

  this->deallocateAndCheckBlockIsThenEmpty(memInBetween);
}


TYPED_TEST(affix_allocatorTest, ThatALargerAllocatedBlockDecreasingReallocatedKeepsThePositionAndTheOldMarker)
{
  this->mem = this->sut.allocate(16);
  auto memInBetween = this->sut.allocate(4);
  auto mem1stPointer = this->mem.ptr;

  EXPECT_TRUE(this->sut.reallocate(this->mem, 8));

  EXPECT_EQ(8, this->mem.length);
  EXPECT_EQ(mem1stPointer, this->mem.ptr);
  this->checkAffixContent();

  this->deallocateAndCheckBlockIsThenEmpty(memInBetween);
}

TYPED_TEST(affix_allocatorTest, ThatAnEmptyBlockedExpandedByZeroBytesIsStillAnEmptyBlock)
{
  EXPECT_TRUE(this->sut.expand(this->mem, 0));
  EXPECT_EQ(0, this->mem.length);
}

TYPED_TEST(affix_allocatorTest, ThatAnEmptyBlockedExpandedByTheSizeOfTheAllocatorsCapacityIsStillAnEmptyBlockBecauseThereIsNoSpaceForTheAffix)
{
  EXPECT_FALSE(this->sut.expand(this->mem, 512));
  EXPECT_EQ(0, this->mem.length);
}

TYPED_TEST(affix_allocatorTest, ThatAnEmptyBlockedExpandedIntoTheLimitsOfTheAllocatorBytesHasNowThatSizeAndHasNowMarker)
{
  size_t sizeThatFitsJustIntoTheAllocator = 512 - TypeParam::prefix_size - TypeParam::sufix_size;
  EXPECT_TRUE(this->sut.expand(this->mem, sizeThatFitsJustIntoTheAllocator));
  EXPECT_EQ(sizeThatFitsJustIntoTheAllocator, this->mem.length);
  this->checkAffixContent();
}

TYPED_TEST(affix_allocatorTest, ThatAnFilledBlockedExpandedIntoTheLimitsOfTheAllocatorBytesHasNowThatSizeAndHasNowMarker)
{
  auto firstSize = 8;
  this->mem = this->sut.allocate(firstSize);
  size_t sizeThatFitsJustIntoTheAllocator = 512 - this->mem.length - TypeParam::prefix_size - TypeParam::sufix_size;
  EXPECT_TRUE(this->sut.expand(this->mem, sizeThatFitsJustIntoTheAllocator));
  EXPECT_EQ(sizeThatFitsJustIntoTheAllocator + firstSize, this->mem.length);
  this->checkAffixContent();
}


TYPED_TEST(affix_allocatorTest, ThatAnEmptyBlockedExpandedBy8BytesHasNowThatSizeAndHasNowMarker)
{
  EXPECT_TRUE(this->sut.expand(this->mem, 8));
  EXPECT_EQ(8, this->mem.length);
  this->checkAffixContent();
}

TYPED_TEST(affix_allocatorTest, ThatFilledBlockedExpandedBy16BytesHasNowThatSizeAndHasNowMarker)
{
  this->mem = this->sut.allocate(8);
  EXPECT_TRUE(this->sut.expand(this->mem, 16));
  EXPECT_EQ(24, this->mem.length);
  this->checkAffixContent();
}

