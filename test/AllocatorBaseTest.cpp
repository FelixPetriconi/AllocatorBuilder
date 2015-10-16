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

#include <alb/allocator_base.hpp>

TEST(roundToAlignmentTest, ThatForDifferentAlignmentsTheCorrectValuesAreCalculated)
{
  EXPECT_EQ(0u, alb::internal::round_to_alignment(4, 0));
  EXPECT_EQ(4u, alb::internal::round_to_alignment(4, 1));
  EXPECT_EQ(4u, alb::internal::round_to_alignment(4, 4));
  EXPECT_EQ(8u, alb::internal::round_to_alignment(4, 5));

  EXPECT_EQ(0u, alb::internal::round_to_alignment(8, 0));
  EXPECT_EQ(8u, alb::internal::round_to_alignment(8, 1));
  EXPECT_EQ(8u, alb::internal::round_to_alignment(8, 8));
  EXPECT_EQ(16u, alb::internal::round_to_alignment(8, 9));
}
