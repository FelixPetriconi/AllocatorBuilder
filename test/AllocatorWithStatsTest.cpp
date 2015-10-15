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
#include <alb/allocator_with_stats.hpp>
#include <alb/stack_allocator.hpp>
#include <alb/fallback_allocator.hpp>
#include <alb/mallocator.hpp>
#include "TestHelpers/Base.h"

#include <algorithm>

namespace {
  typedef alb::allocator_with_stats<
      alb::fallback_allocator<alb::stack_allocator<128, 4>, alb::test_helpers::TestMallocator>>
      AllocatorWithStatsThatCanExpand;
}

template <class Allocator> class AllocationExpectation {
  Allocator &allocator;
  template <class A> friend class AllocationExpectationBuilder;

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
  AllocationExpectation(Allocator &a)
    : allocator(a)
    , numAllocate(0)
    , numAllocateOK(0)
    , numExpand(0)
    , numExpandOK(0)
    , numReallocate(0)
    , numReallocateOK(0)
    , numReallocateInPlace(0)
    , numDeallocate(0)
    , numDeallocateAll(0)
    , numOwns(0)
    , bytesAllocated(0)
    , bytesDeallocated(0)
    , bytesExpanded(0)
    , bytesContracted(0)
    , bytesMoved(0)
    , bytesSlack(0)
    , bytesHighTide(0)
  {
  }

  void checkThatExpectationsAreFulfilled()
  {
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

template <class Allocator> class AllocationExpectationBuilder {
  AllocationExpectation<Allocator> _info;

public:
  AllocationExpectationBuilder(Allocator &a)
    : _info(a)
  {
  }
  AllocationExpectationBuilder &withOwns(size_t v)
  {
    _info.numOwns = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumAllocate(size_t v)
  {
    _info.numAllocate = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumAllocateOK(size_t v)
  {
    _info.numAllocateOK = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumExpand(size_t v)
  {
    _info.numExpand = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumExpandOK(size_t v)
  {
    _info.numExpandOK = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumReallocate(size_t v)
  {
    _info.numReallocate = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumReallocateOK(size_t v)
  {
    _info.numReallocateOK = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumReallocateInPlace(size_t v)
  {
    _info.numReallocateInPlace = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumDeallocate(size_t v)
  {
    _info.numDeallocate = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumDeallocateAll(size_t v)
  {
    _info.numDeallocateAll = v;
    return *this;
  }
  AllocationExpectationBuilder &withNumOwns(size_t v)
  {
    _info.numOwns = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesAllocated(size_t v)
  {
    _info.bytesAllocated = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesDeallocated(size_t v)
  {
    _info.bytesDeallocated = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesExpanded(size_t v)
  {
    _info.bytesExpanded = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesContracted(size_t v)
  {
    _info.bytesContracted = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesMoved(size_t v)
  {
    _info.bytesMoved = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesSlack(size_t v)
  {
    _info.bytesSlack = v;
    return *this;
  }
  AllocationExpectationBuilder &withBytesHighTide(size_t v)
  {
    _info.bytesHighTide = v;
    return *this;
  }

  AllocationExpectation<Allocator> build() const
  {
    return _info;
  }
};

template <class Allocator> class AllocatorWithStatsBaseTest : public ::testing::Test {
protected:
  using AllocatorUnderTest = Allocator;
  using CurrentAllocationInfo = typename Allocator::AllocationInfo;

  void deleteAllExpectations(std::vector<CurrentAllocationInfo *> &expectation)
  {
    for (auto &p : expectation) {
      delete p;
    }
  }

  auto createCallerExpectation(const char *file, const char *function, size_t size)
  {
    auto expectedCallerInfo = new CurrentAllocationInfo;
    expectedCallerInfo->callerFile = file;
    expectedCallerInfo->callerFunction = function;
    expectedCallerInfo->callerSize = size;
    return expectedCallerInfo;
  }

  auto extractRealAllocations(const typename AllocatorUnderTest::Allocations &a)
  {
    std::vector<CurrentAllocationInfo *> result;
    std::copy(a.cbegin(), a.cend(), std::back_inserter(result));
    std::reverse(result.begin(), result.end());
    return result;
  }

  void
  checkTheAllocationCallerExpectations(const std::vector<CurrentAllocationInfo *> &expectations,
                                       const std::vector<CurrentAllocationInfo *> &realAllocations)
  {
    EXPECT_TRUE(std::equal(expectations.cbegin(), expectations.cend(), realAllocations.cbegin(),
                           realAllocations.cend(),
                           [](const auto &l, const auto &r) { return *l == *r; }));
  }

  void SetUp()
  {
    sut = std::make_unique<Allocator>();
  }

  void TearDown()
  {
    EXPECT_EQ(0u, alb::test_helpers::TestMallocator::currentlyAllocatedBytes());
  }

  std::unique_ptr<Allocator> sut;
};

class AllocatorWithStatsTest : public AllocatorWithStatsBaseTest<AllocatorWithStatsThatCanExpand> {
};

TEST_F(AllocatorWithStatsTest, ThatAllocatingZeroBytesIsStored)
{
  auto beforeAllocation = AllocationExpectationBuilder<AllocatorUnderTest>(*sut).build();
  beforeAllocation.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut->allocations().empty());

  auto mem = ALLOCATE((*sut), 0);

  auto afterEmptyAllocation =
      AllocationExpectationBuilder<AllocatorUnderTest>(*sut).withNumAllocate(1).build();
  afterEmptyAllocation.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut->allocations().empty());

  sut->deallocate(mem);

  auto afterEmptyDeallocation = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                    .withNumAllocate(1)
                                    .withNumDeallocate(1)
                                    .build();
  afterEmptyDeallocation.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut->allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatAllocatingAnAlignedNumerOfBytesIsStored)
{
  std::unique_ptr<CurrentAllocationInfo> expectedCallerInfo(
      createCallerExpectation(__FILE__, __FUNCTION__, 4));
  auto mem = ALLOCATE((*sut), 4);
  expectedCallerInfo->callerLine = __LINE__;

  auto afterAllocationOf4Bytes = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                     .withNumAllocate(1)
                                     .withNumAllocateOK(1)
                                     .withBytesAllocated(4)
                                     .withBytesHighTide(4)
                                     .build();

  afterAllocationOf4Bytes.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(*expectedCallerInfo == **(allocations.cbegin()));
  }

  sut->deallocate(mem);

  auto afterDeallocationOfThe4Bytes = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                          .withNumAllocate(1)
                                          .withNumAllocateOK(1)
                                          .withNumDeallocate(1)
                                          .withBytesAllocated(4)
                                          .withBytesDeallocated(4)
                                          .withBytesHighTide(4)
                                          .build();

  afterDeallocationOfThe4Bytes.checkThatExpectationsAreFulfilled();

  EXPECT_TRUE(sut->allocations().empty());
}

TEST_F(AllocatorWithStatsTest, ThatTwoAllocationsAreStoredAndThatTheCallerStatsAreCorrect)
{
  std::vector<CurrentAllocationInfo *> expectedCallerInfo;
  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 4));
  auto mem1st = ALLOCATE((*sut), 4);
  expectedCallerInfo[0]->callerLine = __LINE__;

  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 8));
  auto mem2nd = ALLOCATE((*sut), 8);
  expectedCallerInfo[1]->callerLine = __LINE__;

  auto afterAllocationOf4And8Bytes = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                         .withNumAllocate(2)
                                         .withNumAllocateOK(2)
                                         .withBytesAllocated(12)
                                         .withBytesHighTide(12)
                                         .build();

  afterAllocationOf4And8Bytes.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }

  sut->deallocate(mem1st);

  auto afterDeallocationOf4BytesAndStillKeeping8Bytes =
      AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
          .withNumAllocate(2)
          .withNumAllocateOK(2)
          .withNumDeallocate(1)
          .withBytesAllocated(12)
          .withBytesDeallocated(4)
          .withBytesHighTide(12)
          .build();

  afterDeallocationOf4BytesAndStillKeeping8Bytes.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());
    auto realAllocations = extractRealAllocations(allocations);
    EXPECT_TRUE(*expectedCallerInfo[1] == *realAllocations[0]);
  }

  sut->deallocate(mem2nd);

  auto afterDeallocationEverything = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                         .withNumAllocate(2)
                                         .withNumAllocateOK(2)
                                         .withNumDeallocate(2)
                                         .withBytesAllocated(12)
                                         .withBytesDeallocated(12)
                                         .withBytesHighTide(12)
                                         .build();

  afterDeallocationEverything.checkThatExpectationsAreFulfilled();

  EXPECT_TRUE(sut->allocations().empty());

  deleteAllExpectations(expectedCallerInfo);
}

TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingInPlaceIsStored)
{
  std::unique_ptr<CurrentAllocationInfo> expectedCallerInfo(
      createCallerExpectation(__FILE__, __FUNCTION__, 4));
  auto mem = ALLOCATE((*sut), 4);
  expectedCallerInfo->callerLine = __LINE__;

  EXPECT_TRUE(sut->reallocate(mem, 16));

  auto afterReallocating4BytesTo16 = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                         .withNumAllocate(1)
                                         .withNumAllocateOK(1)
                                         .withNumReallocate(1)
                                         .withNumReallocateOK(1)
                                         .withNumReallocateInPlace(1)
                                         .withBytesAllocated(16)
                                         .withBytesExpanded(12)
                                         .withBytesHighTide(16)
                                         .build();

  afterReallocating4BytesTo16.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(*expectedCallerInfo == **(allocations.cbegin()));
  }

  sut->deallocate(mem);

  auto afterDeallocatingAll = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                  .withNumAllocate(1)
                                  .withNumAllocateOK(1)
                                  .withNumDeallocate(1)
                                  .withNumReallocate(1)
                                  .withNumReallocateOK(1)
                                  .withNumReallocateInPlace(1)
                                  .withBytesAllocated(16)
                                  .withBytesExpanded(12)
                                  .withBytesDeallocated(16)
                                  .withBytesHighTide(16)
                                  .build();

  afterDeallocatingAll.checkThatExpectationsAreFulfilled();
  EXPECT_TRUE(sut->allocations().empty());
}

TEST_F(AllocatorWithStatsTest,
       ThatTheDeallocationOfThe2ndAllocationOfThreeLeavesThe1stAnd3rdAllocationsInPlace)
{
  std::vector<CurrentAllocationInfo *> expectedCallerInfo;
  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 4));
  auto mem1st = ALLOCATE((*sut), 4);
  expectedCallerInfo[0]->callerLine = __LINE__;

  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 8));
  auto mem2nd = ALLOCATE((*sut), 8);
  expectedCallerInfo[1]->callerLine = __LINE__;

  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 12));
  auto mem3rd = ALLOCATE((*sut), 12);
  expectedCallerInfo[2]->callerLine = __LINE__;

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }

  sut->deallocate(mem2nd);

  {
    expectedCallerInfo.erase(expectedCallerInfo.begin() + 1);

    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }

  sut->deallocate(mem1st);
  sut->deallocate(mem3rd);
  EXPECT_TRUE(sut->allocations().empty());

  deleteAllExpectations(expectedCallerInfo);
}

TEST_F(AllocatorWithStatsTest, ThatIncreasingReallocatingNotInPlaceIsStored)
{
  std::vector<CurrentAllocationInfo *> expectedCallerInfo;
  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 4));
  auto mem1st = ALLOCATE((*sut), 4);
  expectedCallerInfo[0]->callerLine = __LINE__;

  expectedCallerInfo.push_back(createCallerExpectation(__FILE__, __FUNCTION__, 8));
  auto mem2nd = ALLOCATE((*sut), 8);
  expectedCallerInfo[1]->callerLine = __LINE__;

  EXPECT_TRUE(sut->reallocate(mem2nd, 128));

  auto afterReallocating8BytesTo128 = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                          .withNumAllocate(2)
                                          .withNumAllocateOK(2)
                                          .withNumReallocate(1)
                                          .withNumReallocateOK(1)
                                          .withBytesAllocated(140)
                                          .withBytesDeallocated(8)
                                          .withBytesMoved(8)
                                          .withBytesHighTide(132)
                                          .build();

  afterReallocating8BytesTo128.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());

    auto realAllocations = extractRealAllocations(allocations);
    checkTheAllocationCallerExpectations(expectedCallerInfo, realAllocations);
  }

  sut->deallocate(mem1st);

  auto afterDeallocatingTheFirstBlock = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                            .withNumAllocate(2)
                                            .withNumAllocateOK(2)
                                            .withNumDeallocate(1)
                                            .withNumReallocate(1)
                                            .withNumReallocateOK(1)
                                            .withBytesAllocated(140)
                                            .withBytesDeallocated(12)
                                            .withBytesMoved(8)
                                            .withBytesHighTide(132)
                                            .build();

  afterDeallocatingTheFirstBlock.checkThatExpectationsAreFulfilled();

  {
    const auto allocations = sut->allocations();
    EXPECT_FALSE(allocations.empty());
    EXPECT_TRUE(*expectedCallerInfo[1] == **(allocations.cbegin()));
  }

  sut->deallocate(mem2nd);
  EXPECT_TRUE(sut->allocations().empty());

  deleteAllExpectations(expectedCallerInfo);
}

TEST_F(AllocatorWithStatsTest, ThatDecreasingReallocatingInPlaceIsStored)
{
  auto mem = sut->allocate(32);

  EXPECT_TRUE(sut->reallocate(mem, 16));

  auto afterReallocating32BytesTo16 = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                          .withNumAllocate(1)
                                          .withNumAllocateOK(1)
                                          .withNumReallocate(1)
                                          .withNumReallocateOK(1)
                                          .withNumReallocateInPlace(1)
                                          .withBytesAllocated(32)
                                          .withBytesDeallocated(16)
                                          .withBytesContracted(16)
                                          .withBytesHighTide(32)
                                          .build();
  afterReallocating32BytesTo16.checkThatExpectationsAreFulfilled();

  sut->deallocate(mem);

  auto afterDeallocatingEverything = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                         .withNumAllocate(1)
                                         .withNumAllocateOK(1)
                                         .withNumDeallocate(1)
                                         .withNumReallocate(1)
                                         .withNumReallocateOK(1)
                                         .withNumReallocateInPlace(1)
                                         .withBytesAllocated(32)
                                         .withBytesDeallocated(32)
                                         .withBytesContracted(16)
                                         .withBytesHighTide(32)
                                         .build();
  afterDeallocatingEverything.checkThatExpectationsAreFulfilled();
}

TEST_F(AllocatorWithStatsTest, ThatSuccessfulExpandingIsStored)
{
  auto mem = sut->allocate(4);

  EXPECT_TRUE(sut->expand(mem, 16));

  auto afterExpanding4To16Bytes = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                      .withNumAllocate(1)
                                      .withNumAllocateOK(1)
                                      .withNumExpand(1)
                                      .withNumExpandOK(1)
                                      .withBytesAllocated(20)
                                      .withBytesExpanded(16)
                                      .withBytesHighTide(20)
                                      .build();
  afterExpanding4To16Bytes.checkThatExpectationsAreFulfilled();

  sut->deallocate(mem);
  auto afterDeallocatingEveything = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                        .withNumAllocate(1)
                                        .withNumAllocateOK(1)
                                        .withNumExpand(1)
                                        .withNumExpandOK(1)
                                        .withNumDeallocate(1)
                                        .withBytesAllocated(20)
                                        .withBytesExpanded(16)
                                        .withBytesDeallocated(20)
                                        .withBytesHighTide(20)
                                        .build();
  afterDeallocatingEveything.checkThatExpectationsAreFulfilled();
}

class AllocatorWithStatsWithLimitedExpandingTest
    : public AllocatorWithStatsBaseTest<alb::allocator_with_stats<alb::stack_allocator<64, 4>>> {
};

TEST_F(AllocatorWithStatsWithLimitedExpandingTest, ThatFailedReallocationIsStored)
{
  auto mem = sut->allocate(4);

  EXPECT_FALSE(sut->reallocate(mem, 128));

  auto afterAFailedReallocationOf4To128Bytes =
      AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
          .withNumAllocate(1)
          .withNumAllocateOK(1)
          .withNumReallocate(1)
          .withBytesAllocated(4)
          .withBytesHighTide(4)
          .build();

  afterAFailedReallocationOf4To128Bytes.checkThatExpectationsAreFulfilled();

  sut->deallocate(mem);

  auto afterDeallocationEverything = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                         .withNumAllocate(1)
                                         .withNumAllocateOK(1)
                                         .withNumDeallocate(1)
                                         .withNumReallocate(1)
                                         .withBytesAllocated(4)
                                         .withBytesDeallocated(4)
                                         .withBytesHighTide(4)
                                         .build();
  afterDeallocationEverything.checkThatExpectationsAreFulfilled();
}

TEST_F(AllocatorWithStatsWithLimitedExpandingTest, ThatFailedExpandingIsStored)
{
  auto mem = sut->allocate(4);

  EXPECT_FALSE(sut->expand(mem, 64));

  auto afterAFailedExpansionOf4To64Bytes = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                               .withNumAllocate(1)
                                               .withNumAllocateOK(1)
                                               .withNumExpand(1)
                                               .withBytesAllocated(4)
                                               .withBytesHighTide(4)
                                               .build();

  afterAFailedExpansionOf4To64Bytes.checkThatExpectationsAreFulfilled();

  sut->deallocate(mem);
  auto afterDeallocatinEverything = AllocationExpectationBuilder<AllocatorUnderTest>(*sut)
                                        .withNumAllocate(1)
                                        .withNumAllocateOK(1)
                                        .withNumDeallocate(1)
                                        .withNumExpand(1)
                                        .withBytesAllocated(4)
                                        .withBytesDeallocated(4)
                                        .withBytesHighTide(4)
                                        .build();

  afterDeallocatinEverything.checkThatExpectationsAreFulfilled();
}
