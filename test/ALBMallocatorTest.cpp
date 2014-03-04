///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
//////////////////////////////////////////////////////////////////
#include <gtest/gtest.h>
#include "ALBTestHelpers.h"
#include "mallocator.hpp"
#include "ALBTestHelpersAllocatorBaseTest.h"

using namespace ALB::TestHelpers;

class MallocatorTest : public AllocatorBaseTest<ALB::Mallocator> {
protected:
  void TearDown() {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
  ALB::Block mem;
};

TEST_F(MallocatorTest, ThatAllocatingZeroBytesResultsInAnEmptyBlock)
{
  mem = sut.allocate(0);
  EXPECT_FALSE(mem);
}

TEST_F(MallocatorTest, ThatAllocatingResultsInAnCorrectBock)
{
  mem = sut.allocate(8);
  EXPECT_EQ(8, mem.length);
}

#ifdef NDEBUG // cannot be tested in debug mode
TEST_F(MallocatorTest, ThatAllocatingATooHugeBlockResultsIntoAnEmptyBlock)
{
  mem = sut.allocate(-1);
  EXPECT_FALSE(mem);
}
#endif

TEST_F(MallocatorTest, ThatReallocatingResultsInAnNewSizedBlock)
{
  mem = sut.allocate(8);
  EXPECT_TRUE(sut.reallocate(mem, 16));
  EXPECT_EQ(16, mem.length);
}

TEST_F(MallocatorTest, ThatReallocatingABlockToZeroResultsInAnEmptyBlock)
{
  mem = sut.allocate(8);
  EXPECT_TRUE(sut.reallocate(mem, 0));
  EXPECT_FALSE(mem);
}

#ifdef NDEBUG // cannot be tested in debug mode
TEST_F(MallocatorTest, ThatReallocatingABlockToATooHugeFails)
{
  mem = sut.allocate(8);
  EXPECT_FALSE(sut.reallocate(mem, -1));
  EXPECT_EQ(8, mem.length);
}
#endif