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

class AffixAllocatorWithPrefixTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
    ALB::AffixAllocator<
      ALB::StackAllocator<512>, 
      ALB::MemoryCorruptionDetector<int, 0xbaadf00d>, 
      ALB::MemoryCorruptionDetector<int, 0xf000baaa>>>
{

};

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
  EXPECT_EQ(0xbaadf00d, *(static_cast<int*>(mem.ptr) - 1) ); 
  EXPECT_EQ(0xf000baaa, *(static_cast<int*>(mem.ptr) + 8 / sizeof(int)) );

  deallocateAndCheckBlockIsThenEmpty(mem);
}
