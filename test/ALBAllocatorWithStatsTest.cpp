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
  > AllocatorWithStatsThatCanExpand;
}

template <class Allocator>
class AllocationExpectation {
  Allocator& allocator;
  template <class Allocator> friend class AllocationExpectationBuilder;

  size_t owns;
  size_t numAllocate;
  size_t numAllocateOK;
  size_t numExpand;
  size_t numExpandOK;
  size_t numReallocate;
  size_t numReallocateOK;
  size_t numReallocateInPlace;
  size_t numDeallocate;
  size_t numDeallocateAll;
  size_t numOwns;
  size_t bytesAllocated;
  size_t bytesDeallocated;
  size_t bytesExpanded;
  size_t bytesContracted;
  size_t bytesMoved;
  size_t bytesSlack;
  size_t bytesHighTide;

public:
  AllocationExpectation(Allocator& a)
    : allocator(a)
    , numOwns(0)
    , numAllocate(0)
    , numAllocateOK(0)
    , numExpand(0)
    , numExpandOK(0)
    , numReallocate(0)
    , numReallocateOK(0)
    , numReallocateInPlace(0)
    , numDeallocate(0)
    , numDeallocateAll(0)
    , bytesAllocated(0)
    , bytesDeallocated(0)
    , bytesExpanded(0)
    , bytesContracted(0)
    , bytesMoved(0)
    , bytesSlack(0)
    , bytesHighTide(0)
  {}

  void checkThatExpectationsAreFulfilled() {
    EXPECT_EQ(numOwns, allocator.numOwns());
    EXPECT_EQ(numAllocate, allocator.numAllocate());
    EXPECT_EQ(numAllocateOK, allocator.numAllocateOK());
    EXPECT_EQ(numExpand, allocator.numExpand());
    EXPECT_EQ(numExpandOK, allocator.numExpandOK());
    EXPECT_EQ(numReallocate, allocator.numReallocate());
    EXPECT_EQ(numReallocateOK, allocator.numReallocateOK());
    EXPECT_EQ(numReallocateInPlace, allocator.numReallocateInPlace());
    EXPECT_EQ(numDeallocate, allocator.numDeallocate());
    EXPECT_EQ(numDeallocateAll, allocator.numDeallocateAll());
    EXPECT_EQ(bytesAllocated, allocator.bytesAllocated());
    EXPECT_EQ(bytesDeallocated, allocator.bytesDeallocated());
    EXPECT_EQ(bytesExpanded, allocator.bytesExpanded());
    EXPECT_EQ(bytesContracted, allocator.bytesContracted());
    EXPECT_EQ(bytesMoved, allocator.bytesMoved());
    EXPECT_EQ(bytesSlack, allocator.bytesSlack());
    EXPECT_EQ(bytesHighTide, allocator.bytesHighTide());
  }

};

template <class Allocator>
class AllocationExpectationBuilder {
  AllocationExpectation<Allocator> _info;

public:
  AllocationExpectationBuilder(Allocator& a) : _info(a) {}
  AllocationExpectationBuilder& withOwns(size_t v)                { _info.numOwns = v; return *this; }
  AllocationExpectationBuilder& withNumAllocate(size_t v)         { _info.numAllocate = v; return *this; }
  AllocationExpectationBuilder& withNumAllocateOK(size_t v)        { _info.numAllocateOK = v; return *this; }
  AllocationExpectationBuilder& withNumExpand(size_t v)            { _info.numExpand = v; return *this; }
  AllocationExpectationBuilder& withNumExpandOK(size_t v)          { _info.numExpandOK = v; return *this; }
  AllocationExpectationBuilder& withNumReallocate(size_t v)        { _info.numReallocate = v; return *this; }
  AllocationExpectationBuilder& withNumReallocateOK(size_t v)      { _info.numReallocateOK = v; return *this; }
  AllocationExpectationBuilder& withNumReallocateInPlace(size_t v) { _info.numReallocateInPlace = v; return *this; }
  AllocationExpectationBuilder& withNumDeallocate(size_t v)        { _info.numDeallocate = v; return *this; }
  AllocationExpectationBuilder& withNumDeallocateAll(size_t v)     { _info.numDeallocateAll = v; return *this; }
  AllocationExpectationBuilder& withNumOwns(size_t v)              { _info.numOwns = v; return *this; }
  AllocationExpectationBuilder& withBytesAllocated(size_t v)       { _info.bytesAllocated = v; return *this; }
  AllocationExpectationBuilder& withBytesDeallocated(size_t v)     { _info.bytesDeallocated = v; return *this; }
  AllocationExpectationBuilder& withBytesExpanded(size_t v)        { _info.bytesExpanded = v; return *this; }
  AllocationExpectationBuilder& withBytesContracted(size_t v)      { _info.bytesContracted = v; return *this; }
  AllocationExpectationBuilder& withBytesMoved(size_t v)           { _info.bytesMoved = v; return *this; }
  AllocationExpectationBuilder& withBytesSlack(size_t v)           { _info.bytesSlack = v; return *this; }
  AllocationExpectationBuilder& withBytesHighTide(size_t v)        { _info.bytesHighTide = v; return *this; }

  AllocationExpectation<Allocator> build() const { return _info; }
};

template <class Allocator>
class AllocatorWithStatsBaseTest : public ::testing::Test
{
protected:
  typedef Allocator AllocatorUnderTest;

  typename Allocator::AllocationInfo createCallerExpectation(const char* file, const char* function, size_t size) {
    Allocator::AllocationInfo expectedCallerInfo;
    expectedCallerInfo.callerFile = file;
    expectedCallerInfo.callerFunction = function;
    expectedCallerInfo.callerSize = size;
    return expectedCallerInfo;
  }

  std::vector<typename Allocator::AllocationInfo> extractRealAllocations(const typename Allocator::Allocations& a) {
    std::vector<typename Allocator::AllocationInfo> result;
    std::copy(a.cbegin(), a.cend(), std::back_inserter(result));
    std::reverse(result.begin(), result.end());
    return result;
  }

  void checkTheAllocationCallerExpectations(const std::vector<typename Allocator::AllocationInfo>& expectations,
    const std::vector<typename Allocator::AllocationInfo>& realAllocations) {
    EXPECT_TRUE(std::equal(std::begin(expectations), 
      std::end(expectations), 
      std::begin(realAllocations),
      [](const typename Allocator::AllocationInfo& lhs, const typename Allocator::AllocationInfo& rhs) {
        return rhs == lhs;  }));
  }

  Allocator sut;
};

class AllocatorWithStatsTest : public AllocatorWithStatsBaseTest<AllocatorWithStatsThatCanExpand>
{

};

TEST_F(AllocatorWithStatsTest, ThatAllocatingZeroBytesIsStored)
{
  auto beforeAllocation = AllocationExpectationBuilder<AllocatorUnderTest>(sut).build();
  beforeAllocation.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut.allocations().empty());

  auto mem = ALLOCATE(sut, 0); 

  auto afterEmptyAllocation = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut).withNumAllocate(1).build();
  afterEmptyAllocation.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut.allocations().empty());

  sut.deallocate(mem);

  auto afterEmptyDeallocation = AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumDeallocate(1).build();
  afterEmptyDeallocation.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut.allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatAllocatingAnAlignedNumerOfBytesIsStored)
{
  auto expectedCallerInfo = createCallerExpectation(__FILE__, __FUNCTION__, 4);
  auto mem = ALLOCATE(sut, 4); expectedCallerInfo.callerLine = __LINE__;
  
  auto afterAllocationOf4Bytes = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
      .withNumAllocate(1)
      .withNumAllocateOK(1)
      .withBytesAllocated(4)
      .withBytesHighTide(4).build();

  afterAllocationOf4Bytes.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(expectedCallerInfo == *(allocations.cbegin()));
  }

  sut.deallocate(mem);

  auto afterDeallocationOfThe4Bytes = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumDeallocate(1)
    .withBytesAllocated(4)
    .withBytesDeallocated(4)
    .withBytesHighTide(4).build();
  
  afterDeallocationOfThe4Bytes.checkThatExpectationsAreFulfilled();

  EXPECT_TRUE(sut.allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatTwoAllocationsAreStoredAndThatTheCallerStatsAreCorrect)
{
  std::vector<AllocatorUnderTest::AllocationInfo> expectedCallerInfo;
  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 4) );
  auto mem1st = ALLOCATE(sut, 4); expectedCallerInfo[0].callerLine = __LINE__;

  expectedCallerInfo.push_back( createCallerExpectation(__FILE__, __FUNCTION__, 8) );
  auto mem2nd = ALLOCATE(sut, 8); expectedCallerInfo[1].callerLine = __LINE__;

  auto afterAllocationOf4And8Bytes = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(2)
    .withNumAllocateOK(2)
    .withBytesAllocated(12)
    .withBytesHighTide(12).build();

  afterAllocationOf4And8Bytes.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    
    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }
  
  sut.deallocate(mem1st);

  auto afterDeallocationOf4BytesAndStillKeeping8Bytes = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(2)
    .withNumAllocateOK(2)
    .withNumDeallocate(1)
    .withBytesAllocated(12)
    .withBytesDeallocated(4)
    .withBytesHighTide(12).build();

  afterDeallocationOf4BytesAndStillKeeping8Bytes.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(expectedCallerInfo[1] == realAllocations[0]);
  }

  sut.deallocate(mem2nd);

  auto afterDeallocationEverything = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(2)
    .withNumAllocateOK(2)
    .withNumDeallocate(2)
    .withBytesAllocated(12)
    .withBytesDeallocated(12)
    .withBytesHighTide(12).build();

  afterDeallocationEverything.checkThatExpectationsAreFulfilled();

  EXPECT_TRUE(sut.allocations().empty());
}


TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingInPlaceIsStored)
{
  auto expectedCallerInfo = createCallerExpectation(__FILE__, __FUNCTION__, 4);
  auto mem = ALLOCATE(sut, 4); expectedCallerInfo.callerLine = __LINE__;

  EXPECT_TRUE(sut.reallocate(mem, 16));

  auto afterReallocating4BytesTo16 = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumReallocate(1)
    .withNumReallocateOK(1)
    .withNumReallocateInPlace(1)
    .withBytesAllocated(16)
    .withBytesExpanded(12)
    .withBytesHighTide(16).build();

  afterReallocating4BytesTo16.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(expectedCallerInfo == *(allocations.cbegin()));
  }

  sut.deallocate(mem);

  auto afterDeallocatingAll = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumDeallocate(1)
    .withNumReallocate(1)
    .withNumReallocateOK(1)
    .withNumReallocateInPlace(1)
    .withBytesAllocated(16)
    .withBytesExpanded(12)
    .withBytesDeallocated(16)
    .withBytesHighTide(16).build();

  afterDeallocatingAll.checkThatExpectationsAreFulfilled();
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
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }

  sut.deallocate(mem2nd);

  {
    expectedCallerInfo.erase(expectedCallerInfo.begin() + 1);

    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
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

  auto afterReallocating8BytesTo128 = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(2)
    .withNumAllocateOK(2)
    .withNumReallocate(1)
    .withNumReallocateOK(1)
    .withBytesAllocated(140)
    .withBytesDeallocated(8)
    .withBytesMoved(8)
    .withBytesHighTide(132).build();
  
  afterReallocating8BytesTo128.checkThatExpectationsAreFulfilled();


  {
    const auto allocations = sut.allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }

  sut.deallocate(mem1st);

  auto afterDeallocatingTheFirstBlock = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(2)
    .withNumAllocateOK(2)
    .withNumDeallocate(1)
    .withNumReallocate(1)
    .withNumReallocateOK(1)
    .withBytesAllocated(140)
    .withBytesDeallocated(12)
    .withBytesMoved(8)
    .withBytesHighTide(132).build();

  afterDeallocatingTheFirstBlock.checkThatExpectationsAreFulfilled();

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
  
  auto afterReallocating32BytesTo16 = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumReallocate(1)
    .withNumReallocateOK(1)
    .withNumReallocateInPlace(1)
    .withBytesAllocated(32)
    .withBytesDeallocated(16)
    .withBytesContracted(16)
    .withBytesHighTide(32).build();
  afterReallocating32BytesTo16.checkThatExpectationsAreFulfilled();

  sut.deallocate(mem);

  auto afterDeallocatingEverything = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumDeallocate(1)
    .withNumReallocate(1)
    .withNumReallocateOK(1)
    .withNumReallocateInPlace(1)
    .withBytesAllocated(32)
    .withBytesDeallocated(32)
    .withBytesContracted(16)
    .withBytesHighTide(32).build();
  afterDeallocatingEverything.checkThatExpectationsAreFulfilled();
}

TEST_F(AllocatorWithStatsTest, ThatSuccessfulExpandingIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_TRUE(sut.expand(mem, 16));
  
  auto afterExpanding4To16Bytes = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumExpand(1)
    .withNumExpandOK(1)
    .withBytesAllocated(20)
    .withBytesExpanded(16)
    .withBytesHighTide(20).build();
  afterExpanding4To16Bytes.checkThatExpectationsAreFulfilled();

  sut.deallocate(mem);
  auto afterDeallocatingEveything= 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumExpand(1)
    .withNumExpandOK(1)
    .withNumDeallocate(1)
    .withBytesAllocated(20)
    .withBytesExpanded(16)
    .withBytesDeallocated(20)
    .withBytesHighTide(20).build();
  afterDeallocatingEveything.checkThatExpectationsAreFulfilled();
}

class AllocatorWithStatsWithLimitedExpandingTest : 
  public AllocatorWithStatsBaseTest<ALB::AllocatorWithStats<ALB::StackAllocator<64, 4>>>
{
};

TEST_F(AllocatorWithStatsWithLimitedExpandingTest, ThatFailedReallocationIsStored)
{
  auto mem = sut.allocate(4);

  EXPECT_FALSE(sut.reallocate(mem, 128));

  auto afterAFailedReallocationOf4To128Bytes = 
    AllocationExpectationBuilder<AllocatorUnderTest>(sut)
    .withNumAllocate(1)
    .withNumAllocateOK(1)
    .withNumReallocate(1)
    .withBytesAllocated(4)
    .withBytesHighTide(4).build();

  afterAFailedReallocationOf4To128Bytes.checkThatExpectationsAreFulfilled();

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




