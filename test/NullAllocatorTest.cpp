///////////////////////////////////////////////////////////////////
//
// Copyright 2015 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>
#include <alb/null_allocator.hpp>

TEST(NullAllocatorTest, AllocationAllocatesNothing)
{
  alb::null_allocator sut;
  auto b = sut.allocate(16);
  EXPECT_FALSE(b);
}
