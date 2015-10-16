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
#include <alb/shared_heap.hpp>
#include <alb/heap.hpp>
#include <alb/mallocator.hpp>
#include <alb/affix_allocator.hpp>
#include "TestHelpers/AllocatorBaseTest.h"
#include "TestHelpers/UsedMemGenerator.h"
#include "TestHelpers/Thread.h"
#include "TestHelpers/AffixGuard.h"
#include "TestHelpers/Base.h"

using namespace alb::test_helpers;

namespace {
  const size_t SmallChunkSize = 8;
  const size_t NumberOfChunks = 192;
}

template <class T> class HeapWithSmallAllocationsTest : public AllocatorBaseTest<T> {
};

using TypesForHeapTest = ::testing::Types<alb::shared_heap<alb::mallocator, NumberOfChunks, SmallChunkSize>,
                         alb::heap<alb::mallocator, NumberOfChunks, SmallChunkSize>>;

TYPED_TEST_CASE(HeapWithSmallAllocationsTest, TypesForHeapTest);

TYPED_TEST(HeapWithSmallAllocationsTest, ThatAllocatingZeroBytesReturnsAnEmptyMemoryBlock)
{
  auto mem = this->sut.allocate(0);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_FALSE(mem);

  EXPECT_NO_THROW(this->sut.deallocate(mem));

  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithSmallAllocationsTest, ThatAllocatingOneBytesReturnsABlockOfBlockSize)
{
  auto mem = this->sut.allocate(1);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(SmallChunkSize, mem.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithSmallAllocationsTest, ThatAllocatingBlockSizeBytesReturnsABlockOfBlockSize)
{
  auto mem = this->sut.allocate(SmallChunkSize);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(SmallChunkSize, mem.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithSmallAllocationsTest, ThatAllocatingTwoMemoryBlocksUsesContiguousMemory)
{
  auto mem1stAllocation = this->sut.allocate(SmallChunkSize);
  auto mem2ndAllocation = this->sut.allocate(SmallChunkSize);

  EXPECT_EQ(mem2ndAllocation.ptr, static_cast<char *>(mem1stAllocation.ptr) + SmallChunkSize);
  EXPECT_EQ(SmallChunkSize, mem1stAllocation.length);
  EXPECT_EQ(SmallChunkSize, mem2ndAllocation.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem2ndAllocation);
  this->deallocateAndCheckBlockIsThenEmpty(mem1stAllocation);
}

TYPED_TEST(HeapWithSmallAllocationsTest, ThatAFreedBlockIsUsedForANewAllocation)
{
  auto mem1stAllocation = this->sut.allocate(SmallChunkSize);
  auto mem2ndAllocation = this->sut.allocate(SmallChunkSize);

  auto ptrOf1stLocation = mem1stAllocation.ptr;
  this->deallocateAndCheckBlockIsThenEmpty(mem1stAllocation);

  auto mem3rdAllocation = this->sut.allocate(SmallChunkSize);
  EXPECT_EQ(ptrOf1stLocation, mem3rdAllocation.ptr);

  this->deallocateAndCheckBlockIsThenEmpty(mem2ndAllocation);
  this->deallocateAndCheckBlockIsThenEmpty(mem3rdAllocation);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatItsPossibleToAllocateAllMemoryBlocksAndThatAllOfThemAreContiguous)
{
  alb::block blocks[NumberOfChunks];

  for (auto &b : blocks) {
    b = this->sut.allocate(8);
  }

  for (size_t i = 0; i < NumberOfChunks - 1; i++) {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i + 1].ptr != nullptr);
    EXPECT_EQ(blocks[i + 1].ptr, static_cast<char *>(blocks[i].ptr) + SmallChunkSize);
  }

  for (auto &b : blocks) {
    this->deallocateAndCheckBlockIsThenEmpty(b);
  }
}

TYPED_TEST(HeapWithSmallAllocationsTest, ThatANullBlockIsReturnedIfTheHeapIsOutOfMemory)
{
  auto allMem = this->sut.allocate(NumberOfChunks * SmallChunkSize);

  auto outOfMem = this->sut.allocate(1);

  EXPECT_EQ(nullptr, outOfMem.ptr);
  EXPECT_EQ(0u, outOfMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatANullBlockIsReturnedIfABiggerChunkIsReqeustedThanTheCompleteHeapCanHandle)
{
  auto outOfMem = this->sut.allocate(NumberOfChunks * SmallChunkSize + 1);

  EXPECT_EQ(nullptr, outOfMem.ptr);
  EXPECT_EQ(0u, outOfMem.length);
}

TYPED_TEST(
    HeapWithSmallAllocationsTest,
    ThatItsPossibleToAllocateHalfNumberOfChunksMemoryBlocksWithDoubleBlockSizeBytesAndThatAllOfThemAreContiguous)
{
  alb::block blocks[NumberOfChunks / 2];

  for (auto &b : blocks) {
    b = this->sut.allocate(SmallChunkSize * 2);
  }

  for (size_t i = 0; i < NumberOfChunks / 2 - 1; i++) {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i + 1].ptr != nullptr);
    EXPECT_EQ(blocks[i + 1].ptr, static_cast<char *>(blocks[i].ptr) + SmallChunkSize * 2);
  }

  for (auto &b : blocks) {
    this->deallocateAndCheckBlockIsThenEmpty(b);
  }
}

TYPED_TEST(HeapWithSmallAllocationsTest, ThatABiggerFreedBlockIsUsedForANewAllocationOfSmallerSize)
{
  auto largerAllocation = this->sut.allocate(SmallChunkSize * 2);
  auto smallerAllocation = this->sut.allocate(SmallChunkSize);

  auto ptrOf1stLocation = largerAllocation.ptr;
  this->deallocateAndCheckBlockIsThenEmpty(largerAllocation);

  auto mem = this->sut.allocate(SmallChunkSize);
  EXPECT_EQ(ptrOf1stLocation, mem.ptr);

  this->deallocateAndCheckBlockIsThenEmpty(smallerAllocation);
  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatASmallerFreedBlockIsNotUsedForANewAllocationButANewContiguousBlockIsUsed)
{
  auto mem1 = this->sut.allocate(SmallChunkSize);
  auto mem2 = this->sut.allocate(SmallChunkSize * 2);

  auto ptrOf1stLocation = mem1.ptr;
  this->deallocateAndCheckBlockIsThenEmpty(mem1);

  auto mem3 = this->sut.allocate(SmallChunkSize * 2);

  ASSERT_NE(ptrOf1stLocation, mem3.ptr);
  EXPECT_EQ(static_cast<char *>(mem3.ptr), static_cast<char *>(mem2.ptr) + SmallChunkSize * 2);

  this->deallocateAndCheckBlockIsThenEmpty(mem2);
  this->deallocateAndCheckBlockIsThenEmpty(mem3);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandByZeroBytesOfAnEmptyBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = this->sut.allocate(0);

  EXPECT_TRUE(this->sut.expand(mem, 0));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandByZeroBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = this->sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.expand(mem, 0));

  EXPECT_EQ(origMem, mem);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), mem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandByNBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = this->sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.expand(mem, 4));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_GE(mem.length, origMem.length + 4);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandingOfABlockIsWithinAUsedAreaThatFitsIntoTheGapIsSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 2>(this->sut)
                     .withAUsedPatternOf("1100'0110")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.expand(mem, SmallChunkSize * 2));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_GE(mem.length, origMem.length + SmallChunkSize);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandingOfABlockIsWithinAUsedAreaThatDoesNotFitsIntoTheGapIsNotSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 2>(this->sut)
                     .withAUsedPatternOf("1100'0110")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 6);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_FALSE(this->sut.expand(mem, SmallChunkSize));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(mem.length, origMem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandingOfABlockBeyondTheSizeOfASingleControlRegsiterIsSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 4>(this->sut)
                     .withAUsedPatternOf("1000'0000'0000'0000")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.expand(mem, SmallChunkSize * 64));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 64), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatExpandingOfABlockBeyondTheSizeOfATwoControlRegisterIsSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 4>(this->sut)
                     .withAUsedPatternOf("1111'1111'1111'1100")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.expand(mem, SmallChunkSize * 128));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 128), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatReallocatrByZeroBytesOfAnEmptyBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = this->sut.allocate(0);

  EXPECT_TRUE(this->sut.reallocate(mem, 0));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatReallocateByZeroBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = this->sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.reallocate(mem, SmallChunkSize));

  EXPECT_EQ(origMem, mem);
  EXPECT_EQ(SmallChunkSize, mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), mem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatReallocateByNBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = this->sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.reallocate(mem, SmallChunkSize + 4));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize + SmallChunkSize, mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatReallocatrOfABlockIsWithinAUsedAreaThatFitsIntoTheGapIsSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 2>(this->sut)
                     .withAUsedPatternOf("1100'0110")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.reallocate(mem, SmallChunkSize * (4 + 2)));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 2), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(
    HeapWithSmallAllocationsTest,
    ThatReallocateOfABlockIsWithinAUsedAreaThatDoesNotFitsIntoTheGapIsSuccessfulWithANewLocations)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 2>(this->sut)
                     .withAUsedPatternOf("1100'0110")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 6);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.reallocate(mem, SmallChunkSize * (6 + 1)));

  EXPECT_NE(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (6 + 1), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatReallocateOfABlockBeyondTheSizeOfASingleControlRegsiterIsSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 4>(this->sut)
                     .withAUsedPatternOf("1000'0000'0000'0000")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.reallocate(mem, SmallChunkSize * (4 + 64)));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 64), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

TYPED_TEST(HeapWithSmallAllocationsTest,
           ThatReallocateOfABlockBeyondTheSizeOfATwoControlRegisterIsSuccessful)
{
  auto usedMem = UsedMemGenerator<TypeParam, SmallChunkSize * 4>(this->sut)
                     .withAUsedPatternOf("1111'1111'1111'1100")
                     .build();

  auto mem = this->sut.allocate(SmallChunkSize * 4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(this->sut.reallocate(mem, SmallChunkSize * (128 + 4)));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 128), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void *)ReferenceData.data(), origMem.length);
}

template <class T> class HeapWithLargeAllocationsTest : public AllocatorBaseTest<T> {
};

typedef ::testing::Types<alb::shared_heap<alb::mallocator, 512, 8>,
                         alb::heap<alb::mallocator, 512, 8>> TypesForLargeHeapTest;

TYPED_TEST_CASE(HeapWithLargeAllocationsTest, TypesForLargeHeapTest);

TYPED_TEST(HeapWithLargeAllocationsTest, ThatAfterAFilledChunkTheNextIsStarted)
{
  auto mem1 = this->sut.allocate(8 * 64);
  auto mem2 = this->sut.allocate(8);

  EXPECT_EQ(static_cast<char *>(mem2.ptr), static_cast<char *>(mem1.ptr) + 8 * 64);
  auto mem3 = this->sut.allocate(8 * 64);
  EXPECT_EQ(static_cast<char *>(mem3.ptr), static_cast<char *>(mem1.ptr) + 2 * 8 * 64);

  this->deallocateAndCheckBlockIsThenEmpty(mem1);
  this->deallocateAndCheckBlockIsThenEmpty(mem2);
}

TYPED_TEST(HeapWithLargeAllocationsTest, ThatAllocatingSeveralWholeChunksIsSupported)
{
  auto mem = this->sut.allocate(8 * 128);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(8u * 128, mem.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithLargeAllocationsTest, ThatAllocatingAllWithOneAllocationIsSupported)
{
  auto mem = this->sut.allocate(8 * 512);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(8u * 512, mem.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithLargeAllocationsTest,
           ThatAllocatingSeveralWholeChunksAfterASingleBlockIsSupported)
{
  auto mem1 = this->sut.allocate(1);
  auto mem2 = this->sut.allocate(8 * 128);
  EXPECT_EQ(mem2.ptr, static_cast<char *>(mem1.ptr) + 8 * 64);
  EXPECT_EQ(8u * 128, mem2.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem1);
  this->deallocateAndCheckBlockIsThenEmpty(mem2);
}

TYPED_TEST(
    HeapWithLargeAllocationsTest,
    ThatWhenAllChunksAreUsedNoMemoryCanBeAllocatedAndThatDeallocateAllFreesMakesAllBlocksAgainAvailable)
{
  alb::block blocks[8];
  for (auto &b : blocks) {
    b = this->sut.allocate(64 * 8);
  }

  auto mem = this->sut.allocate(1);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0u, mem.length);

  this->sut.deallocate_all();
  for (auto &b : blocks) {
    b = this->sut.allocate(64 * 8);
    EXPECT_NE(nullptr, b.ptr);
    EXPECT_EQ(64u * 8, b.length);
  }

  for (auto &b : blocks) {
    this->deallocateAndCheckBlockIsThenEmpty(b);
  }
}

TYPED_TEST(HeapWithLargeAllocationsTest, ThatAllocatingMemoryBiggerThanAChunkSizeWorks)
{
  auto mem = this->sut.allocate(65 * 8);

  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(65u * 8, mem.length);

  this->deallocateAndCheckBlockIsThenEmpty(mem);
}

TYPED_TEST(HeapWithLargeAllocationsTest,
           ThatAllocatingMemoryBiggerThanAChunkSizeWorksAfterASingleMemoryBlockHasTheCorrectOffsets)
{
  auto mem1 = this->sut.allocate(8);
  auto mem2 = this->sut.allocate(65 * 8);
  auto mem3 = this->sut.allocate(56);
  auto mem4 = this->sut.allocate(8);

  EXPECT_EQ(mem2.ptr, static_cast<char *>(mem1.ptr) +
                          8 * 8); // These big blocks currently are aligned on 8-BlockBorder
  EXPECT_EQ(mem3.ptr, static_cast<char *>(mem1.ptr) + 8); // This fits into the first gap
  EXPECT_EQ(mem4.ptr,
            static_cast<char *>(mem2.ptr) +
                65 * 8); // This comes after the big chunk, because there is no gap inbetween

  this->deallocateAndCheckBlockIsThenEmpty(mem1);
  this->deallocateAndCheckBlockIsThenEmpty(mem2);
  this->deallocateAndCheckBlockIsThenEmpty(mem3);
  this->deallocateAndCheckBlockIsThenEmpty(mem4);
}

class SharedHeapTreatedWithThreadsTest : public ::testing::Test {
};

TEST_F(SharedHeapTreatedWithThreadsTest, BruteForceAllocationByOneRunningThread)
{
  using AllocatorUnderTest = alb::shared_heap<alb::mallocator, 512, 8>;
  AllocatorUnderTest sut;

  TestWorker<AllocatorUnderTest> testWorker(sut, 128);
  testWorker.check();

  auto allDeallocatedCheck = sut.allocate(512 * 8);
  EXPECT_TRUE((bool)allDeallocatedCheck);
}

TEST_F(SharedHeapTreatedWithThreadsTest,
       BruteForceTestWithSeveralThreadsRunningHoldingASingleAllocation)
{
  using AllocatorUnderTest = alb::shared_heap<alb::mallocator, 512, 64>;
  AllocatorUnderTest sut;

  typedef std::array<unsigned char, 2> TestParams;
  TestParams maxAllocatedBytes = {127, 131};

  TestWorkerCollector<AllocatorUnderTest, 2, TestWorker<AllocatorUnderTest>, TestParams>
      testCollector(sut, maxAllocatedBytes);

  testCollector.check();

  auto allDeallocatedCheck = sut.allocate(512 * 64);
  EXPECT_TRUE((bool)allDeallocatedCheck);
}

TEST_F(SharedHeapTreatedWithThreadsTest,
       BruteForceTestWith4ThreadsRunningHoldingMultipleAllocations)
{
  const size_t NumberOfChunks = 1024;
  const size_t BlockSize = 64;
  const size_t NumberOfThread = 4;

  using PrefixGuard = AffixGuard<unsigned, 0xbaadf00d>;
  using  SufixGuard = AffixGuard<unsigned, 0xf000baaa>;

  using AllocatorUnderTest = alb::affix_allocator<alb::shared_heap<alb::mallocator, NumberOfChunks, BlockSize>,
                               PrefixGuard, SufixGuard>;

  AllocatorUnderTest sut;

  using TestParams = std::array<unsigned char, NumberOfThread>;
  TestParams maxAllocatedBytes = {127, 131, 165, 129};

  TestWorkerCollector<AllocatorUnderTest, NumberOfThread,
                      MultipleAllocationsTester<AllocatorUnderTest>,
                      TestParams> testCollector(sut, maxAllocatedBytes);

  testCollector.check();

  // At the end, we check that all memory was released. The easiest way to check this
  // is by trying to allocate everything
  auto allDeallocatedCheck =
      sut.allocate(NumberOfChunks * BlockSize - AllocatorUnderTest::prefix_size -
                   AllocatorUnderTest::sufix_size);

  EXPECT_TRUE(static_cast<bool>(allDeallocatedCheck));
}
