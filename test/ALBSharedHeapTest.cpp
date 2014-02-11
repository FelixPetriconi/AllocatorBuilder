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
#include <gmock/gmock.h>
#include "ALBSharedHeap.h"
#include "ALBMallocator.h"
#include "ALBTestHelpers.h"
#include <thread>

class HeapTestWithOneBlockChunk : public ::testing::Test
{
protected:
 ALB::SharedHeap<ALB::Mallocator, 64, 8> sut;
};

TEST_F(HeapTestWithOneBlockChunk, ThatAllocatingZeroBytesReturnsAnEmptyMemoryBlock)
{
  auto mem = sut.allocate(0);
  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);

  EXPECT_NO_THROW(sut.deallocate(mem));
}

TEST_F(HeapTestWithOneBlockChunk, ThatAllocatingOneBytesReturnsABlockOfBlockSize)
{
  auto mem = sut.allocate(1);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(8, mem.length);

  EXPECT_NO_THROW(sut.deallocate(mem));
}

TEST_F(HeapTestWithOneBlockChunk, ThatAllocatingBlockSizeBytesReturnsABlockOfBlockSize)
{
  auto mem = sut.allocate(8);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(8, mem.length);

  EXPECT_NO_THROW(sut.deallocate(mem));
}


TEST_F(HeapTestWithOneBlockChunk, ThatAllocatingTwoMemoryBlocksUsesContiguousMemory)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);
  EXPECT_EQ(mem2.ptr, static_cast<char*>(mem1.ptr) + 8);
  EXPECT_EQ(8, mem1.length);
  EXPECT_EQ(8, mem2.length);

  EXPECT_NO_THROW(sut.deallocate(mem2));
  EXPECT_NO_THROW(sut.deallocate(mem1));
}

TEST_F(HeapTestWithOneBlockChunk, ThatAFreedBlockIsUsedForANewAllocation)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(8);

  auto ptrOf1stLocation = mem1.ptr;
  EXPECT_NO_THROW(sut.deallocate(mem1));

  auto mem3 = sut.allocate(8);
  EXPECT_EQ(ptrOf1stLocation, mem3.ptr);

  EXPECT_NO_THROW(sut.deallocate(mem2));
  EXPECT_NO_THROW(sut.deallocate(mem3));
}

TEST_F(HeapTestWithOneBlockChunk, ThatItsPossibleToAllocate64MemoryBlocksAndThatAllOfThemAreContiguous)
{
  ALB::Block blocks[64];

  for (auto& b : blocks) { b = sut.allocate(8); }

  for (size_t i = 0; i < 63; i++)
  {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i+1].ptr != nullptr);
    EXPECT_EQ(blocks[i+1].ptr, static_cast<char*>(blocks[i].ptr) + 8);
  }

  for (auto& b : blocks) { sut.deallocate(b); }
}

TEST_F(HeapTestWithOneBlockChunk, ThatANullBlockIsreturnedIfTheHeapIsOutOfMemory)
{
  ALB::Block blocks[64];
  for (auto& b : blocks) { b = sut.allocate(8); }

  auto mem = sut.allocate(8);

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);
}

TEST_F(HeapTestWithOneBlockChunk, ThatANullBlockIsreturnedIfABiggerChunkIsReqeustedThanTheCompleteHeapCanHandle)
{
  auto mem = sut.allocate(64 * 8 + 1);

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);
}


TEST_F(HeapTestWithOneBlockChunk, ThatItsPossibleToAllocate32MemoryBlocksWith16BytesAndThatAllOfThemAreContiguous)
{
  ALB::Block blocks[32];

  for (auto& b : blocks) { b = sut.allocate(16); }

  for (size_t i = 0; i < 31; i++)
  {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i+1].ptr != nullptr);
    EXPECT_EQ(blocks[i+1].ptr, static_cast<char*>(blocks[i].ptr) + 16);
  }

  for (auto& b : blocks) { sut.deallocate(b); }
}


TEST_F(HeapTestWithOneBlockChunk, ThatABiggerFreedBlockIsUsedForANewAllocationOfSmallerSize)
{
  auto mem1 = sut.allocate(16);
  auto mem2 = sut.allocate(8);

  auto ptrOf1stLocation = mem1.ptr;
  EXPECT_NO_THROW(sut.deallocate(mem1));

  auto mem3 = sut.allocate(8);
  EXPECT_EQ(ptrOf1stLocation, mem3.ptr);

  EXPECT_NO_THROW(sut.deallocate(mem2));
  EXPECT_NO_THROW(sut.deallocate(mem3));
}

TEST_F(HeapTestWithOneBlockChunk, ThatASmallerFreedBlockIsNotUsedForANewAllocationButANewContiguousBlockIsUsed)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(16);

  auto ptrOf1stLocation = mem1.ptr;
  EXPECT_NO_THROW(sut.deallocate(mem1));

  auto mem3 = sut.allocate(16);
  ASSERT_NE(ptrOf1stLocation, mem3.ptr);
  EXPECT_EQ(static_cast<char*>(mem3.ptr), static_cast<char*>(mem2.ptr) + 16);

  EXPECT_NO_THROW(sut.deallocate(mem2));
  EXPECT_NO_THROW(sut.deallocate(mem3));
}


class HeapWithSeveralChunksTest : public ::testing::Test
{
protected:
 ALB::SharedHeap<ALB::Mallocator, 512, 8> sut;
};

TEST_F(HeapWithSeveralChunksTest, ThatAfterAFilledChunkTheNextIsStarted)
{
  auto mem1 = sut.allocate(8*64);
  auto mem2 = sut.allocate(8);

  EXPECT_EQ(static_cast<char*>(mem2.ptr), static_cast<char*>(mem1.ptr) + 8*64);
  auto mem3 = sut.allocate(8*64);
  EXPECT_EQ(static_cast<char*>(mem3.ptr), static_cast<char*>(mem1.ptr) + 2*8*64);

  EXPECT_NO_THROW(sut.deallocate(mem1));
  EXPECT_NO_THROW(sut.deallocate(mem2));
}

TEST_F(HeapWithSeveralChunksTest, ThatAllocatingSeveralWholeChunksIsSupported)
{
  auto mem = sut.allocate(8*128);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(8*128, mem.length);

  EXPECT_NO_THROW(sut.deallocate(mem));
}

TEST_F(HeapWithSeveralChunksTest, ThatAllocatingAllWithOneAllocationIsSupported)
{
  auto mem = sut.allocate(8*512);
  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(8*512, mem.length);

  EXPECT_NO_THROW(sut.deallocate(mem));
}

TEST_F(HeapWithSeveralChunksTest, ThatAllocatingSeveralWholeChunksAfterASingleBlockIsSupported)
{
  auto mem1 = sut.allocate(1);
  auto mem2 = sut.allocate(8*128);
  EXPECT_EQ(mem2.ptr, static_cast<char*>(mem1.ptr) + 8*64);
  EXPECT_EQ(8*128, mem2.length);

  EXPECT_NO_THROW(sut.deallocate(mem1));
  EXPECT_NO_THROW(sut.deallocate(mem2));
}


TEST_F(HeapWithSeveralChunksTest, ThatWhenAllChunksAreUsedNoMemoryCanBeAllocatedAndThatDeallocateAllFreesMakesAllBlocksAgainAvailable)
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

  for (auto& b : blocks) { sut.deallocate(b); }
}

TEST_F(HeapWithSeveralChunksTest, ThatAllocatingMemoryBiggerThanAChunkSizeWorks)
{
  auto mem = sut.allocate(65*8);

  EXPECT_NE(nullptr, mem.ptr);
  EXPECT_EQ(65*8, mem.length);

  EXPECT_NO_THROW(sut.deallocate(mem));
}

TEST_F(HeapWithSeveralChunksTest, ThatAllocatingMemoryBiggerThanAChunkSizeWorksAfterASingleMemoryBlockHasTheCorrectOffsets)
{
  auto mem1 = sut.allocate(8);
  auto mem2 = sut.allocate(65*8);
  auto mem3 = sut.allocate(56);
  auto mem4 = sut.allocate(8);

  EXPECT_EQ(mem2.ptr, static_cast<char*>(mem1.ptr) + 8*8); // These big blocks currently are aligned on 8-BlockBorder
  EXPECT_EQ(mem3.ptr, static_cast<char*>(mem1.ptr) + 8);   // This fits into the first gab
  EXPECT_EQ(mem4.ptr, static_cast<char*>(mem2.ptr) + 65*8); // This comes after the big chunk, because there is no gap inbetween

  EXPECT_NO_THROW(sut.deallocate(mem1));
  EXPECT_NO_THROW(sut.deallocate(mem2));
  EXPECT_NO_THROW(sut.deallocate(mem3));
  EXPECT_NO_THROW(sut.deallocate(mem4));
}


class HeapWithThreadsTest : public ::testing::Test
{
};

TEST_F(HeapWithThreadsTest,  BruteForceAllocationByOneRunningThread)
{
  typedef ALB::SharedHeap<ALB::Mallocator, 512, 8> AllocatorUnderTest;
  AllocatorUnderTest sut;

  ALB::TestHelpers::TestWorker<AllocatorUnderTest> testWorker(sut, 128);
  testWorker.check();

  auto allDeallocatedCheck = sut.allocate(512*8);
  EXPECT_TRUE((bool)allDeallocatedCheck);
}


TEST_F(HeapWithThreadsTest, BruteForceTestWithSeveralThreadsRunning)
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