///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
///////////////////////////////////////////////////////////////////
#pragma once

#include <gtest/gtest.h>
#include <alb/allocator_base.hpp>

namespace alb
{
namespace test_helpers
{

template <class Allocator>
class AllocatorBaseTest : public ::testing::Test
{
protected:
  typedef Allocator allocator;

  void deallocateAndCheckBlockIsThenEmpty(alb::Block& b) {
    sut.deallocate(b);
    EXPECT_FALSE(b);
    EXPECT_EQ(nullptr, b.ptr);
  }
  Allocator sut;
};

}
}