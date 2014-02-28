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
#include "ALBAffixAllocator.h"
#include "ALBTestHelpers.h"
#include "ALBStackAllocator.h"
#include "ALBMemmoryCorruptionDetector.h"
#include "ALBTestHelpersAllocatorBaseTest.h"

namespace
{
  const unsigned PrefixMarker = 0xbaadf00d;
  const uint64_t LargePrefixMarker = 0xfefefefebaadf00d;
  const unsigned SufixMarker = 0xf000baaa;
}

template <class T>
class AffixAllocatorTest : public ALB::TestHelpers::AllocatorBaseTest<T> 
{
protected:
  void TearDown() {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }

  void checkAffixContent() {
    if (T::prefix_size > 0) {
      EXPECT_EQ(T::prefix::pattern, *(static_cast<T::prefix::value_type*>(mem.ptr) - 1) ) 
        << "Problem with type " << typeid(T).name();
    }
    if (T::sufix_size > 0) {
      EXPECT_EQ(T::sufix::pattern, *(static_cast<T::sufix::value_type*>(mem.ptr) + mem.length / sizeof(T::sufix::value_type)) ) 
        << "Problem with type " << typeid(T).name();
    }
  }
  ALB::Block mem;
};

typedef ::testing::Types<
  ALB::AffixAllocator<
    ALB::StackAllocator<512,4>, 
      ALB::MemoryCorruptionDetector<unsigned, PrefixMarker>
  >,
  ALB::AffixAllocator<
    ALB::StackAllocator<512,4>, 
      ALB::AffixAllocatorHelper::NoAffix,
      ALB::MemoryCorruptionDetector<unsigned, SufixMarker>
  >,
  ALB::AffixAllocator<
    ALB::StackAllocator<512, 4>, 
      ALB::MemoryCorruptionDetector<unsigned, PrefixMarker>,
      ALB::MemoryCorruptionDetector<unsigned, SufixMarker>
  >,
  ALB::AffixAllocator<
    ALB::StackAllocator<512, 4>, 
      ALB::MemoryCorruptionDetector<uint64_t, LargePrefixMarker>,
        ALB::MemoryCorruptionDetector<unsigned, SufixMarker>
  >
> TypesToTest;

TYPED_TEST_CASE(AffixAllocatorTest, TypesToTest);


TYPED_TEST(AffixAllocatorTest, ThatAnEmptyAllocationReturnsAnEmptyBlock)
{
  mem = sut.allocate(0);

  EXPECT_EQ(0, mem.length);
  EXPECT_EQ(nullptr, mem.ptr);
}

TYPED_TEST(AffixAllocatorTest, ThatAFullAllocationReturnsAnEmptyBlockBecauseOfMissingSpaceForTheAffixes)
{
  mem = sut.allocate(512);

  EXPECT_EQ(0, mem.length);
  EXPECT_EQ(nullptr, mem.ptr);
}

TYPED_TEST(AffixAllocatorTest, ThatASmallAllocationsReturnsTheRequestedSizeAndThatTheMarkerAreSetCorrectly)
{
  mem = sut.allocate(8);
  
  EXPECT_EQ(8, mem.length);

  checkAffixContent();
}

TYPED_TEST(AffixAllocatorTest, ThatASmallAllocatedBlockIncreasingReallocatedIntoANewMemAreaKeepsTheOldMarker)
{
  mem = sut.allocate(8);
  auto memInBetween = sut.allocate(4);

  EXPECT_TRUE(sut.reallocate(mem, 16));
  
  EXPECT_EQ(16, mem.length);
  checkAffixContent();

  deallocateAndCheckBlockIsThenEmpty(memInBetween);
}


TYPED_TEST(AffixAllocatorTest, ThatALargerAllocatedBlockDecreasingReallocatedKeepsThePositionAndTheOldMarker)
{
  mem = sut.allocate(16);
  auto memInBetween = sut.allocate(4);
  auto mem1stPointer = mem.ptr;

  EXPECT_TRUE(sut.reallocate(mem, 8));

  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(mem1stPointer, mem.ptr);
  checkAffixContent();

  deallocateAndCheckBlockIsThenEmpty(memInBetween);
}

TYPED_TEST(AffixAllocatorTest, ThatAnEmptyBlockedExpandedByZeroBytesIsStillAnEmptyBlock)
{
  EXPECT_TRUE(sut.expand(mem, 0));
  EXPECT_EQ(0, mem.length);
}

TYPED_TEST(AffixAllocatorTest, ThatAnEmptyBlockedExpandedByTheSizeOfTheAllocatorsCapacityIsStillAnEmptyBlockBecauseThereIsNoSpaceForTheAffix)
{
  EXPECT_FALSE(sut.expand(mem, 512));
  EXPECT_EQ(0, mem.length);
}

TYPED_TEST(AffixAllocatorTest, ThatAnEmptyBlockedExpandedIntoTheLimitsOfTheAllocatorBytesHasNowThatSizeAndHasNowMarker)
{
  size_t sizeThatFitsJustIntoTheAllocator = 512 - TypeParam::prefix_size - TypeParam::sufix_size;
  EXPECT_TRUE(sut.expand(mem, sizeThatFitsJustIntoTheAllocator));
  EXPECT_EQ(sizeThatFitsJustIntoTheAllocator, mem.length);
  checkAffixContent();
}

TYPED_TEST(AffixAllocatorTest, ThatAnFilledBlockedExpandedIntoTheLimitsOfTheAllocatorBytesHasNowThatSizeAndHasNowMarker)
{
  auto firstSize = 8;
  mem = sut.allocate(firstSize);
  size_t sizeThatFitsJustIntoTheAllocator = 512 - mem.length - TypeParam::prefix_size - TypeParam::sufix_size;
  EXPECT_TRUE(sut.expand(mem, sizeThatFitsJustIntoTheAllocator));
  EXPECT_EQ(sizeThatFitsJustIntoTheAllocator + firstSize, mem.length);
  checkAffixContent();
}


TYPED_TEST(AffixAllocatorTest, ThatAnEmptyBlockedExpandedBy8BytesHasNowThatSizeAndHasNowMarker)
{
  EXPECT_TRUE(sut.expand(mem, 8));
  EXPECT_EQ(8, mem.length);
  checkAffixContent();
}

TYPED_TEST(AffixAllocatorTest, ThatFilledBlockedExpandedBy16BytesHasNowThatSizeAndHasNowMarker)
{
  mem = sut.allocate(8);
  EXPECT_TRUE(sut.expand(mem, 16));
  EXPECT_EQ(24, mem.length);
  checkAffixContent();
}

