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
#include "ALBAllocatorWithStats.h"
#include "ALBStackAllocator.h"
#include "ALBFallbackAllocator.h"
#include "ALBMallocator.h"

class AllocatorWithStatsTest : public ::testing::Test
{
public:
  ALB::AllocatorWithStats<
    ALB::FallbackAllocator<
      ALB::StackAllocator<64, 4>, 
      ALB::Mallocator
    >
  > sut;
};

TEST_F(AllocatorWithStatsTest, ThatAllocatingZeroBytesIsStored)
{
  EXPECT_EQ(0, sut.numAllocate());
  EXPECT_EQ(0, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(0, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(0, sut.bytesHighTide());

  auto mem = sut.allocate(0);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(0, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(0, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(0, sut.bytesHighTide());

  sut.deallocate(mem);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(0, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(0, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(0, sut.bytesHighTide());
}

TEST_F(AllocatorWithStatsTest, ThatAllocatingAnAlignedNumerOfBytesIsStored)
{
  auto mem = sut.allocate(4);
  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(4, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(4, sut.bytesHighTide());

  sut.deallocate(mem);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(4, sut.bytesAllocated());
  EXPECT_EQ(4, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(4, sut.bytesHighTide());
}

TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingInPlaceIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_TRUE(sut.reallocate(mem, 16));

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(1, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(16, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(12, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(16, sut.bytesHighTide());

  sut.deallocate(mem);
  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(1, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(16, sut.bytesAllocated());
  EXPECT_EQ(16, sut.bytesDeallocated());
  EXPECT_EQ(12, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(16, sut.bytesHighTide());
}

TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingNotInPlaceIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_TRUE(sut.reallocate(mem, 128));
  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(132, sut.bytesAllocated());
  EXPECT_EQ(4, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(4, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(128, sut.bytesHighTide());

  sut.deallocate(mem);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(132, sut.bytesAllocated());
  EXPECT_EQ(132, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(4, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(128, sut.bytesHighTide());
}

TEST_F(AllocatorWithStatsTest, ThatDecreasingReallocatingInPlaceIsStored)
{
  auto mem = sut.allocate(32);

  EXPECT_TRUE(sut.reallocate(mem, 16));

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(1, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(32, sut.bytesAllocated());
  EXPECT_EQ(16, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(16, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(32, sut.bytesHighTide());

  sut.deallocate(mem);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(1, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(32, sut.bytesAllocated());
  EXPECT_EQ(32, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(16, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(32, sut.bytesHighTide());
}

TEST_F(AllocatorWithStatsTest, ThatSuccessfulExpandingIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_TRUE(sut.expand(mem, 16));

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(1, sut.numExpand());
  EXPECT_EQ(1, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(20, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(16, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(20, sut.bytesHighTide());

  sut.deallocate(mem);
  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(1, sut.numExpand());
  EXPECT_EQ(1, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(20, sut.bytesAllocated());
  EXPECT_EQ(20, sut.bytesDeallocated());
  EXPECT_EQ(16, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(20, sut.bytesHighTide());
}

class AllocatorWithStatsWithLimitedExpandingTest : public ::testing::Test
{
public:
  ALB::AllocatorWithStats<ALB::StackAllocator<64, 4>> sut;
};

TEST_F(AllocatorWithStatsWithLimitedExpandingTest, ThatFailedReallocationIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_FALSE(sut.reallocate(mem, 128));

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(4, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(4, sut.bytesHighTide());

  sut.deallocate(mem);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(4, sut.bytesAllocated());
  EXPECT_EQ(4, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(4, sut.bytesHighTide());
}

TEST_F(AllocatorWithStatsWithLimitedExpandingTest, ThatFailedExpandingIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_FALSE(sut.expand(mem, 64));

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(1, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(4, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(4, sut.bytesHighTide());

  sut.deallocate(mem);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(1, sut.numAllocateOK());
  EXPECT_EQ(1, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(4, sut.bytesAllocated());
  EXPECT_EQ(4, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(4, sut.bytesHighTide());
}




