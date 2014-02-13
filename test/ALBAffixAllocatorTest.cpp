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

namespace
{
  const unsigned PrefixMarker = 0xbaadf00d;
  const uint64_t LargePrefixMarker = 0xfefefefebaadf00d;
  const unsigned SufixMarker = 0xf000baaa;
}


class AffixAllocatorWithPrefixTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
    ALB::AffixAllocator<
      ALB::StackAllocator<512>, 
      ALB::MemoryCorruptionDetector<unsigned, PrefixMarker>>>
{};

TEST_F(AffixAllocatorWithPrefixTest, ThatAnEmptyAllocationReturnsAnEmptyBlock)
{
  auto mem = sut.allocate(0);

  EXPECT_EQ(0, mem.length);
  EXPECT_EQ(nullptr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(AffixAllocatorWithPrefixTest, ThatAFullAllocationReturnsAnEmptyBlockBecauseOfMissingSpaceForTheAffixes)
{
  auto mem = sut.allocate(512);

  EXPECT_EQ(0, mem.length);
  EXPECT_EQ(nullptr, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(AffixAllocatorWithPrefixTest, ThatASmallAllocationsReturnsTheRequestedSizeAndThatTheMarkerAreSetCorrectly)
{
  auto mem = sut.allocate(8);
  
  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(PrefixMarker, *(static_cast<int*>(mem.ptr) - 1) ); 
  //EXPECT_EQ(SufixMarker, *(static_cast<int*>(mem.ptr) + 8 / sizeof(int)) );

  deallocateAndCheckBlockIsThenEmpty(mem);
}


class AffixAllocatorWithPrefixAndSuffixTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
  ALB::AffixAllocator<
  ALB::StackAllocator<512>, 
  ALB::MemoryCorruptionDetector<unsigned, PrefixMarker>,
  ALB::MemoryCorruptionDetector<unsigned, SufixMarker>>>
{};

TEST_F(AffixAllocatorWithPrefixAndSuffixTest, ThatASmallAllocationsReturnsTheRequestedSizeAndThatTheMarkerAreSetCorrectly)
{
  auto mem = sut.allocate(8);

  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(PrefixMarker, *(static_cast<int*>(mem.ptr) - 1) ); 
  EXPECT_EQ(SufixMarker, *(static_cast<int*>(mem.ptr) + 8 / sizeof(int)) );

  deallocateAndCheckBlockIsThenEmpty(mem);
}

class AffixAllocatorWithDifferentSizedPrefixAndSuffixTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
  ALB::AffixAllocator<
  ALB::StackAllocator<512>, 
  ALB::MemoryCorruptionDetector<uint64_t, LargePrefixMarker>,
  ALB::MemoryCorruptionDetector<unsigned, SufixMarker>>>
{};

TEST_F(AffixAllocatorWithDifferentSizedPrefixAndSuffixTest, ThatASmallAllocationsReturnsTheRequestedSizeAndThatTheMarkerAreSetCorrectly)
{
  auto mem = sut.allocate(8);

  EXPECT_EQ(8, mem.length);
  EXPECT_EQ(LargePrefixMarker, *(static_cast<uint64_t*>(mem.ptr) - 1) ); 
  EXPECT_EQ(SufixMarker, *(static_cast<int*>(mem.ptr) + 8 / sizeof(int)) );

  deallocateAndCheckBlockIsThenEmpty(mem);
}
