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

namespace 
{
  typedef ALB::AllocatorWithStats<
    ALB::FallbackAllocator<
    ALB::StackAllocator<128, 4>,
    ALB::Mallocator
    >
  > AllocatorUnderTest;
}

bool operator==(const AllocatorUnderTest::AllocationInfo& lhs, const AllocatorUnderTest::AllocationInfo& rhs) {
  return ::strcmp(lhs.callerFile, rhs.callerFile) == 0 &&
    ::strcmp(lhs.callerFunction, rhs.callerFunction) == 0 &&
    lhs.callerSize == rhs.callerSize;
}


class AllocatorWithStatsTest : public ::testing::Test
{
protected:
  
  AllocatorUnderTest::AllocationInfo createCallerExpectation(const char* file, const char* function, size_t size) {
    AllocatorUnderTest::AllocationInfo expectedCallerInfo;
    expectedCallerInfo.callerFile = file;
    expectedCallerInfo.callerFunction = function;
    expectedCallerInfo.callerSize = size;
    return expectedCallerInfo;
  }

  std::vector<AllocatorUnderTest::AllocationInfo> extractRealAllocations(const AllocatorUnderTest::Allocations& a) {
    std::vector<AllocatorUnderTest::AllocationInfo> result;
    std::copy(a.cbegin(), a.cend(), std::back_inserter(result));
    std::reverse(result.begin(), result.end());
    return result;
  }

  AllocatorUnderTest sut;
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

  EXPECT_TRUE(sut.allocations().empty());

  auto mem = ALLOCATE(sut, 0); 

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
  EXPECT_TRUE(sut.allocations().empty());

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
  EXPECT_TRUE(sut.allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatAllocatingAnAlignedNumerOfBytesIsStored)
{
  auto expectedCallerInfo = createCallerExpectation(__FILE__, __FUNCTION__, 4);
  auto mem = ALLOCATE(sut, 4); expectedCallerInfo.callerLine = __LINE__;

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

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(expectedCallerInfo == *(allocations.cbegin()));
  }

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

  EXPECT_TRUE(sut.allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatTwoAllocationsAreStoredAndThatTheCallerStatsAreCorrect)
{
  std::vector<AllocatorUnderTest::AllocationInfo> expectedCallerInfo;
  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 4) );
  auto mem1st = ALLOCATE(sut, 4); expectedCallerInfo[0].callerLine = __LINE__;

  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 8) );
  auto mem2nd = ALLOCATE(sut, 8); expectedCallerInfo[1].callerLine = __LINE__;

  EXPECT_EQ(2, sut.numAllocate());
  EXPECT_EQ(2, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(12, sut.bytesAllocated());
  EXPECT_EQ(0, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(12, sut.bytesHighTide());

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    
    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(std::equal(std::begin(expectedCallerInfo), 
                          std::end(expectedCallerInfo), 
                          std::begin(realAllocations),
                          [](const AllocatorUnderTest::AllocationInfo& lhs, const AllocatorUnderTest::AllocationInfo& rhs) {
                          return rhs == lhs;  }));
  }
  
  sut.deallocate(mem1st);

  EXPECT_EQ(2, sut.numAllocate());
  EXPECT_EQ(2, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(12, sut.bytesAllocated());
  EXPECT_EQ(4, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(12, sut.bytesHighTide());
  
  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(expectedCallerInfo[1] == realAllocations[0]);
  }

  sut.deallocate(mem2nd);

  EXPECT_EQ(2, sut.numAllocate());
  EXPECT_EQ(2, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(0, sut.numReallocate());
  EXPECT_EQ(0, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(2, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(12, sut.bytesAllocated());
  EXPECT_EQ(12, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(0, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(12, sut.bytesHighTide());

  EXPECT_TRUE(sut.allocations().empty());
}


TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingInPlaceIsStored)
{
  auto expectedCallerInfo = createCallerExpectation(__FILE__, __FUNCTION__, 4);
  auto mem = ALLOCATE(sut, 4); expectedCallerInfo.callerLine = __LINE__;

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

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(expectedCallerInfo == *(allocations.cbegin()));
  }

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

  EXPECT_TRUE(sut.allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatTheDeallocationOfThe2ndAllocationOfThreeLeavesThe1stAnd3rdAllocationsInPlace)
{
  std::vector<AllocatorUnderTest::AllocationInfo> expectedCallerInfo;
  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 4) );
  auto mem1st = ALLOCATE(sut, 4); expectedCallerInfo[0].callerLine = __LINE__;

  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 8) );
  auto mem2nd = ALLOCATE(sut, 8); expectedCallerInfo[1].callerLine = __LINE__;

  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 12) );
  auto mem3rd = ALLOCATE(sut, 12); expectedCallerInfo[2].callerLine = __LINE__;

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(std::equal(std::begin(expectedCallerInfo),
                          std::end(expectedCallerInfo),
                          realAllocations.begin(),
                          [](const AllocatorUnderTest::AllocationInfo& lhs, const AllocatorUnderTest::AllocationInfo& rhs) {
                          return rhs == lhs;  }));
  }

  sut.deallocate(mem2nd);

  {
    expectedCallerInfo.erase(expectedCallerInfo.begin() + 1);

    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(std::equal(std::begin(expectedCallerInfo),
      std::end(expectedCallerInfo),
      realAllocations.begin(),
      [](const AllocatorUnderTest::AllocationInfo& lhs, const AllocatorUnderTest::AllocationInfo& rhs) {
      return rhs == lhs;  }));
  }
}

TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingNotInPlaceIsStored)
{
  std::vector<AllocatorUnderTest::AllocationInfo> expectedCallerInfo;
  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 4));
  auto mem1st = ALLOCATE(sut, 4); expectedCallerInfo[0].callerLine = __LINE__;

  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 8));
  auto mem2nd = ALLOCATE(sut, 8); expectedCallerInfo[1].callerLine = __LINE__;

  EXPECT_TRUE(sut.reallocate(mem2nd, 128));
  EXPECT_EQ(2, sut.numAllocate());
  EXPECT_EQ(2, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(0, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(140, sut.bytesAllocated());
  EXPECT_EQ(8, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(8, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(132, sut.bytesHighTide());

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(std::equal(std::begin(expectedCallerInfo),
      std::end(expectedCallerInfo),
      realAllocations.begin(),
      [](const AllocatorUnderTest::AllocationInfo& lhs, const AllocatorUnderTest::AllocationInfo& rhs) {
      return rhs == lhs;  }));
  }

  sut.deallocate(mem1st);

  EXPECT_EQ(2, sut.numAllocate());
  EXPECT_EQ(2, sut.numAllocateOK());
  EXPECT_EQ(0, sut.numExpand());
  EXPECT_EQ(0, sut.numExpandOK());
  EXPECT_EQ(1, sut.numReallocate());
  EXPECT_EQ(1, sut.numReallocateOK());
  EXPECT_EQ(0, sut.numReallocateInPlace());
  EXPECT_EQ(1, sut.numDeallocate());
  EXPECT_EQ(0, sut.numDeallocateAll());
  EXPECT_EQ(0, sut.numOwns());
  EXPECT_EQ(140, sut.bytesAllocated());
  EXPECT_EQ(12, sut.bytesDeallocated());
  EXPECT_EQ(0, sut.bytesExpanded());
  EXPECT_EQ(0, sut.bytesContracted());
  EXPECT_EQ(8, sut.bytesMoved());
  EXPECT_EQ(0, sut.bytesSlack());
  EXPECT_EQ(132, sut.bytesHighTide());

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(expectedCallerInfo[1] == *(allocations.cbegin()));
  }

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




