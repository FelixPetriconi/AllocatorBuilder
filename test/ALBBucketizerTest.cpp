///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
//////////////////////////////////////////////////////////////////
#pragma once

#include <gtest/gtest.h>
#include "ALBBucketizer.h"
#include "ALBSharedFreeList.h"
#include "ALBMallocator.h"

class BucketizerTest : public ::testing::Test {
protected:
  typedef ALB::Bucketizer<
    ALB::SharedFreeList<
      ALB::Mallocator, ALB::DynamicSetSize, ALB::DynamicSetSize>,
      16, 64, 16> AllocatorUnderTest;

   AllocatorUnderTest sut;
};

TEST_F(BucketizerTest, ThatMinAndMaxSizeOfTheBucketItemsAreSetSpecifiedByTheParameter)
{
  ASSERT_EQ(3, AllocatorUnderTest::number_of_buckets);

  EXPECT_EQ(16, sut._buckets[0].min_size());
  EXPECT_EQ(33, sut._buckets[0].max_size());

  EXPECT_EQ(32, sut._buckets[1].min_size());
  EXPECT_EQ(49, sut._buckets[1].max_size());

  EXPECT_EQ(48, sut._buckets[2].min_size());
  EXPECT_EQ(65, sut._buckets[2].max_size());
}