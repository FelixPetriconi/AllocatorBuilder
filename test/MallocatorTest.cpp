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
#include <alb/mallocator.hpp>
#include "TestHelpers/Base.h"
#include "TestHelpers/AllocatorBaseTest.h"

using namespace alb::test_helpers;

class MallocatorTest : public AllocatorBaseTest<alb::mallocator> {
protected:
  void TearDown()
  {
    deallocateAndCheckBlockIsThenEmpty(mem);
  }
  alb::block mem;
};

TEST_F(MallocatorTest, ThatAllocatingZeroBytesResultsInAnEmptyBlock)
{
  mem = sut.allocate(0);
  EXPECT_FALSE(mem);
}

TEST_F(MallocatorTest, ThatAllocatingResultsInAnCorrectBock)
{
  mem = sut.allocate(8);
  EXPECT_EQ(8u, mem.length);
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
  EXPECT_EQ(16u, mem.length);
}

TEST_F(MallocatorTest, ThatReallocatingABlockToZeroResultsInAnEmptyBlock)
{
  mem = sut.allocate(8);
  EXPECT_TRUE(sut.reallocate(mem, 0));
  EXPECT_FALSE(mem);
}

#if 0
TEST_F(MallocatorTest, ThatReallocatingABlockToATooHugeFails)
{
  mem = sut.allocate(8);
  EXPECT_FALSE(sut.reallocate(mem, std::numeric_limits<size_t>::max()));
  EXPECT_EQ(8u, mem.length);
}
#endif