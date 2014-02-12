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

#include "ALBAllocatorBase.h"

TEST(roundToAlignmentTest, ThatForDifferentAlignmentsTheCorrectValuesAreCalculated)
{
  EXPECT_EQ(0, ALB::Helper::roundToAlignment<4>(0));
  EXPECT_EQ(4, ALB::Helper::roundToAlignment<4>(1));
  EXPECT_EQ(4, ALB::Helper::roundToAlignment<4>(4));
  EXPECT_EQ(8, ALB::Helper::roundToAlignment<4>(5));

  EXPECT_EQ(0, ALB::Helper::roundToAlignment<8>(0));
  EXPECT_EQ(8, ALB::Helper::roundToAlignment<8>(1));
  EXPECT_EQ(8, ALB::Helper::roundToAlignment<8>(8));
  EXPECT_EQ(16, ALB::Helper::roundToAlignment<8>(9));
}
