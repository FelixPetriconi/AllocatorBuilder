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

#include "ALBSharedHeap.h"
#include "ALBMallocator.h"
#include "ALBTestHelpers.h"
#include <thread>
#include <xutility>
#include "ALBAffixAllocator.h"

using namespace ALB::TestHelpers;

namespace
{
   const int ReferenceData[] = {
     0,1,2,3,4,5,6,7,
     8,9,10,11,12,13,14,15,
     16,17,18,19,20,21,22,23,
     24,25,26,27,28,29,30,31
   };

   template <typename T, size_t N>
   size_t size(const T (&)[N]) {
     return N;
   }

   template <typename T>
   void fillBlockWithReferenceData(ALB::Block& b)
   {
     for (size_t i = 0; i < std::min(size(ReferenceData), b.length / sizeof(T)); i++) {
       *(reinterpret_cast<T*>(b.ptr) + i) = ReferenceData[i];
     }
   }
   const size_t SmallBlockSize = 8;
   const size_t NumberOfBlocks = 64;
}



class SharedHeapWithSmallAllocationsTest : 
  public ALB::TestHelpers::AllocatorBaseTest<
    ALB::SharedHeap<ALB::Mallocator, NumberOfBlocks, SmallBlockSize>>
{
};

TEST_F(SharedHeapWithSmallAllocationsTest, ThatAllocatingZeroBytesReturnsAnEmptyMemoryBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_FALSE(mem);

  EXPECT_NO_THROW(sut.deallocate(mem));
  
  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatAllocatingOneBytesReturnsABlockOfBlockSize)
{
  auto mem = sut.allocate(1);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(SmallBlockSize, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatAllocatingBlockSizeBytesReturnsABlockOfBlockSize)
{
  auto mem = sut.allocate(SmallBlockSize);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(SmallBlockSize, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}


TEST_F(SharedHeapWithSmallAllocationsTest, ThatAllocatingTwoMemoryBlocksUsesContiguousMemory)
{
  auto mem1stAllocation = sut.allocate(SmallBlockSize);
  auto mem2ndAllocation = sut.allocate(SmallBlockSize);

  EXPECT_EQ(mem2ndAllocation.ptr, static_cast<char*>(mem1stAllocation.ptr) + SmallBlockSize);
  EXPECT_EQ(SmallBlockSize, mem1stAllocation.length);
  EXPECT_EQ(SmallBlockSize, mem2ndAllocation.length);

  deallocateAndCheckBlockIsThenEmpty(mem2ndAllocation);
  deallocateAndCheckBlockIsThenEmpty(mem1stAllocation);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatAFreedBlockIsUsedForANewAllocation)
{
  auto mem1stAllocation = sut.allocate(SmallBlockSize);
  auto mem2ndAllocation = sut.allocate(SmallBlockSize);

  auto ptrOf1stLocation = mem1stAllocation.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem1stAllocation);

  auto mem3rdAllocation = sut.allocate(SmallBlockSize);
  EXPECT_EQ(ptrOf1stLocation, mem3rdAllocation.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem2ndAllocation);
  deallocateAndCheckBlockIsThenEmpty(mem3rdAllocation);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatItsPossibleToAllocateAllMemoryBlocksAndThatAllOfThemAreContiguous)
{
  ALB::Block blocks[NumberOfBlocks];

  for (auto& b : blocks) { 
    b = sut.allocate(8); 
  }

  for (size_t i = 0; i < NumberOfBlocks - 1; i++)
  {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i+1].ptr != nullptr);
    EXPECT_EQ(blocks[i+1].ptr, static_cast<char*>(blocks[i].ptr) + SmallBlockSize);
  }

  for (auto& b : blocks) { 
    deallocateAndCheckBlockIsThenEmpty(b); 
  }
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatANullBlockIsReturnedIfTheHeapIsOutOfMemory)
{
  auto allMem = sut.allocate(NumberOfBlocks * SmallBlockSize);

  auto outOfMem = sut.allocate(1);

  EXPECT_EQ(nullptr, outOfMem.ptr);
  EXPECT_EQ(0, outOfMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatANullBlockIsReturnedIfABiggerChunkIsReqeustedThanTheCompleteHeapCanHandle)
{
  auto outOfMem = sut.allocate(NumberOfBlocks * SmallBlockSize + 1);

  EXPECT_EQ(nullptr, outOfMem.ptr);
  EXPECT_EQ(0, outOfMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatItsPossibleToAllocateHalfNumberOfBlocksMemoryBlocksWithDoubleBlockSizeBytesAndThatAllOfThemAreContiguous)
{
  ALB::Block blocks[NumberOfBlocks/2];

  for (auto& b : blocks) { 
    b = sut.allocate(SmallBlockSize * 2); 
  }

  for (size_t i = 0; i < NumberOfBlocks/2 - 1; i++)
  {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i+1].ptr != nullptr);
    EXPECT_EQ(blocks[i+1].ptr, static_cast<char*>(blocks[i].ptr) + SmallBlockSize * 2);
  }

  for (auto& b : blocks) { 
    deallocateAndCheckBlockIsThenEmpty(b); 
  }
}


TEST_F(SharedHeapWithSmallAllocationsTest, ThatABiggerFreedBlockIsUsedForANewAllocationOfSmallerSize)
{
  auto largerAllocation = sut.allocate(SmallBlockSize * 2);
  auto smallerAllocation = sut.allocate(SmallBlockSize);

  auto ptrOf1stLocation = largerAllocation.ptr;
  deallocateAndCheckBlockIsThenEmpty(largerAllocation);

  auto mem = sut.allocate(SmallBlockSize);
  EXPECT_EQ(ptrOf1stLocation, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(smallerAllocation);
  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatASmallerFreedBlockIsNotUsedForANewAllocationButANewContiguousBlockIsUsed)
{
  auto mem1 = sut.allocate(SmallBlockSize);
  auto mem2 = sut.allocate(SmallBlockSize * 2);

  auto ptrOf1stLocation = mem1.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem1);

  auto mem3 = sut.allocate(SmallBlockSize * 2);

  ASSERT_NE(ptrOf1stLocation, mem3.ptr);
  EXPECT_EQ(static_cast<char*>(mem3.ptr), static_cast<char*>(mem2.ptr) + SmallBlockSize * 2);

  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem3);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandByZeroBytesOfAnEmptyBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(0);

  EXPECT_TRUE(sut.expand(mem, 0));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandByZeroBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(SmallBlockSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, 0));

  EXPECT_EQ(origMem, mem);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData, mem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandByNBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(SmallBlockSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, 4));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_GE(mem.length, origMem.length + 4);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData, origMem.length);
}


class SharedHeapWithLargeAllocationsTest : public 
  ALB::TestHelpers::AllocatorBaseTest<ALB::SharedHeap<ALB::Mallocator, 512, 8>>
{
};

TEST_F(SharedHeapWithLargeAllocationsTest, ThatAfterAFilledChunkTheNextIsStarted)
{
  auto mem1 = sut.allocate(8*64);
  auto mem2 = sut.allocate(8);

  EXPECT_EQ(static_cast<char*>(mem2.ptr), static_cast<char*>(mem1.ptr) + 8*64);
  auto mem3 = sut.allocate(8*64);
  EXPECT_EQ(static_cast<char*>(mem3.ptr), static_cast<char*>(mem1.ptr) + 2*8*64);

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);
}

TEST_F(SharedHeapWithLargeAllocationsTest, ThatAllocatingSeveralWholeChunksIsSupported)
{
  auto mem = sut.allocate(8*128);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(8*128, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithLargeAllocationsTest, ThatAllocatingAllWithOneAllocationIsSupported)
{
  auto mem = sut.allocate(8*512);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(8*512, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithLargeAllocationsTest, ThatAllocatingSeveralWholeChunksAfterASingleBlockIsSupported)
{
  auto mem1 = sut.allocate(1);
  auto mem2 = sut.allocate(8*128);
  EXPECT_EQ(mem2.ptr, static_cast<char*>(mem1.ptr) + 8*64);
  EXPECT_EQ(8*128, mem2.length);

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);
}


TEST_F(SharedHeapWithLargeAllocationsTest, ThatWhenAllChunksAreUsedNoMemoryCanBeAllocatedAndThatDeallocateAllFreesMakesAllBlocksAgainAvailable)
{
  ALB::Block blocks[8];
  for (auto& b : blocks) { b = sut.allocate(64*8); }

  auto mem = sut.allocate(1);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);

  sut.deallocateAll();
  for (auto& b : blocks) 
  { 
    b = sut.allocate(64*8);
    EXPECT_NE(nullptr, b.ptr);
    EXPECT_EQ(64*8, b.length);
  }

  for (auto& b : blocks) { 
    deallocateAndCheckBlockIsThenEmpty(b); 
  }
}

TEST_F(SharedHeapWithLargeAllocationsTest, ThatAllocatingMemoryBiggerThanAChunkSizeWorks)
{
  auto mem = sut.allocate(65*8);

  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(65*8, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithLargeAllocationsTest, ThatAllocatingMemoryBiggerThanAChunkSizeWorksAfterASingleMemoryBlockHasTheCorrectOffsets)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(65*8);
  auto mem3 = sut.allocate(56);
  auto mem4 = sut.allocate(8);

  EXPECT_EQ(mem2.ptr, static_cast<char*>(mem1.ptr) + 8*8); // These big blocks currently are aligned on 8-BlockBorder
  EXPECT_EQ(mem3.ptr, static_cast<char*>(mem1.ptr) + 8);   // This fits into the first gab
  EXPECT_EQ(mem4.ptr, static_cast<char*>(mem2.ptr) + 65*8); // This comes after the big chunk, because there is no gap inbetween

  deallocateAndCheckBlockIsThenEmpty(mem1);
  deallocateAndCheckBlockIsThenEmpty(mem2);
  deallocateAndCheckBlockIsThenEmpty(mem3);
  deallocateAndCheckBlockIsThenEmpty(mem4);
}




class SharedHeapTreatetWithThreadsTest : public ::testing::Test
{
};

TEST_F(SharedHeapTreatetWithThreadsTest,  BruteForceAllocationByOneRunningThread)
{
  typedef ALB::SharedHeap<ALB::Mallocator, 512, 8> AllocatorUnderTest;
  AllocatorUnderTest sut;

  ALB::TestHelpers::TestWorker<AllocatorUnderTest> testWorker(sut, 128);
  testWorker.check();

  auto allDeallocatedCheck = sut.allocate(512*8);
  EXPECT_TRUE((bool)allDeallocatedCheck);
}


TEST_F(SharedHeapTreatetWithThreadsTest, BruteForceTestWithSeveralThreadsRunningHoldingASingleAllocation)
{
  typedef ALB::SharedHeap<ALB::Mallocator, 512, 64> AllocatorUnderTest;
  AllocatorUnderTest sut;

  typedef std::array<unsigned char, 2> TestParams;
  TestParams maxAllocatedBytes = {127, 131};
  
  ALB::TestHelpers::TestWorkerCollector<
    AllocatorUnderTest, 
    2, 
    ALB::TestHelpers::TestWorker<AllocatorUnderTest>,
    TestParams> testCollector(sut, maxAllocatedBytes);

  testCollector.check();

  auto allDeallocatedCheck = sut.allocate(512*64);
  EXPECT_TRUE((bool)allDeallocatedCheck);
}


TEST_F(SharedHeapTreatetWithThreadsTest, BruteForceTestWith4ThreadsRunningHoldingMultipleAllocations)
{
  const size_t NumberOfBlocks = 1024;
  const size_t BlockSize = 64;
  const size_t NumberOfThread = 4;

  typedef ALB::TestHelpers::AffixGuard<unsigned, 0xbaadf00d> PrefixGuard;
  typedef ALB::TestHelpers::AffixGuard<unsigned, 0xf000baaa> SufixGuard;

  typedef ALB::AffixAllocator<
    ALB::SharedHeap<ALB::Mallocator, NumberOfBlocks, BlockSize>, PrefixGuard, SufixGuard> AllocatorUnderTest;

  AllocatorUnderTest sut;

  typedef std::array<unsigned char, NumberOfThread> TestParams;
  TestParams maxAllocatedBytes = {127, 131, 165, 129};

  ALB::TestHelpers::TestWorkerCollector<
    AllocatorUnderTest, 
    NumberOfThread, 
    ALB::TestHelpers::MultipleAllocationsTester<AllocatorUnderTest>,
    TestParams> testCollector(sut, maxAllocatedBytes);

  testCollector.check();

  // At the end, we check that all memory was released. The easiest way to check this
  // is by trying to allocate everything
  auto allDeallocatedCheck = sut.allocate(
    NumberOfBlocks * BlockSize 
    - AllocatorUnderTest::prefix_size - AllocatorUnderTest::sufix_size);

  EXPECT_TRUE(allDeallocatedCheck);
}
