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

class AllocatorWithStartsTest : public ::testing::Test
{
public:
  ALB::AllocatorWithStats<ALB::StackAllocator<64>> sut;
};

TEST_F(AllocatorWithStartsTest, ThatAllocatingZeroBytesIsStored)
{
  auto mem = sut.allocate(0);

  EXPECT_EQ(1, sut.numAllocate());
  EXPECT_EQ(0, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesHighTide());
}
