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
   const size_t SmallChunkSize = 8;
   const size_t NumberOfChunks = 192;
}



class SharedHeapWithSmallAllocationsTest : 
  public AllocatorBaseTest<
    ALB::SharedHeap<ALB::Mallocator, NumberOfChunks, SmallChunkSize>>
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
  EXPECT_EQ(SmallChunkSize, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatAllocatingBlockSizeBytesReturnsABlockOfBlockSize)
{
  auto mem = sut.allocate(SmallChunkSize);
  EXPECT_TRUE(nullptr != mem.ptr);
  EXPECT_EQ(SmallChunkSize, mem.length);

  deallocateAndCheckBlockIsThenEmpty(mem);
}


TEST_F(SharedHeapWithSmallAllocationsTest, ThatAllocatingTwoMemoryBlocksUsesContiguousMemory)
{
  auto mem1stAllocation = sut.allocate(SmallChunkSize);
  auto mem2ndAllocation = sut.allocate(SmallChunkSize);

  EXPECT_EQ(mem2ndAllocation.ptr, static_cast<char*>(mem1stAllocation.ptr) + SmallChunkSize);
  EXPECT_EQ(SmallChunkSize, mem1stAllocation.length);
  EXPECT_EQ(SmallChunkSize, mem2ndAllocation.length);

  deallocateAndCheckBlockIsThenEmpty(mem2ndAllocation);
  deallocateAndCheckBlockIsThenEmpty(mem1stAllocation);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatAFreedBlockIsUsedForANewAllocation)
{
  auto mem1stAllocation = sut.allocate(SmallChunkSize);
  auto mem2ndAllocation = sut.allocate(SmallChunkSize);

  auto ptrOf1stLocation = mem1stAllocation.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem1stAllocation);

  auto mem3rdAllocation = sut.allocate(SmallChunkSize);
  EXPECT_EQ(ptrOf1stLocation, mem3rdAllocation.ptr);

  deallocateAndCheckBlockIsThenEmpty(mem2ndAllocation);
  deallocateAndCheckBlockIsThenEmpty(mem3rdAllocation);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatItsPossibleToAllocateAllMemoryBlocksAndThatAllOfThemAreContiguous)
{
  ALB::Block blocks[NumberOfChunks];

  for (auto& b : blocks) { 
    b = sut.allocate(8); 
  }

  for (size_t i = 0; i < NumberOfChunks - 1; i++)
  {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i+1].ptr != nullptr);
    EXPECT_EQ(blocks[i+1].ptr, static_cast<char*>(blocks[i].ptr) + SmallChunkSize);
  }

  for (auto& b : blocks) { 
    deallocateAndCheckBlockIsThenEmpty(b); 
  }
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatANullBlockIsReturnedIfTheHeapIsOutOfMemory)
{
  auto allMem = sut.allocate(NumberOfChunks * SmallChunkSize);

  auto outOfMem = sut.allocate(1);

  EXPECT_EQ(nullptr, outOfMem.ptr);
  EXPECT_EQ(0, outOfMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatANullBlockIsReturnedIfABiggerChunkIsReqeustedThanTheCompleteHeapCanHandle)
{
  auto outOfMem = sut.allocate(NumberOfChunks * SmallChunkSize + 1);

  EXPECT_EQ(nullptr, outOfMem.ptr);
  EXPECT_EQ(0, outOfMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatItsPossibleToAllocateHalfNumberOfChunksMemoryBlocksWithDoubleBlockSizeBytesAndThatAllOfThemAreContiguous)
{
  ALB::Block blocks[NumberOfChunks/2];

  for (auto& b : blocks) { 
    b = sut.allocate(SmallChunkSize * 2); 
  }

  for (size_t i = 0; i < NumberOfChunks/2 - 1; i++)
  {
    ASSERT_TRUE(blocks[i].ptr != nullptr);
    ASSERT_TRUE(blocks[i+1].ptr != nullptr);
    EXPECT_EQ(blocks[i+1].ptr, static_cast<char*>(blocks[i].ptr) + SmallChunkSize * 2);
  }

  for (auto& b : blocks) { 
    deallocateAndCheckBlockIsThenEmpty(b); 
  }
}


TEST_F(SharedHeapWithSmallAllocationsTest, ThatABiggerFreedBlockIsUsedForANewAllocationOfSmallerSize)
{
  auto largerAllocation = sut.allocate(SmallChunkSize * 2);
  auto smallerAllocation = sut.allocate(SmallChunkSize);

  auto ptrOf1stLocation = largerAllocation.ptr;
  deallocateAndCheckBlockIsThenEmpty(largerAllocation);

  auto mem = sut.allocate(SmallChunkSize);
  EXPECT_EQ(ptrOf1stLocation, mem.ptr);

  deallocateAndCheckBlockIsThenEmpty(smallerAllocation);
  deallocateAndCheckBlockIsThenEmpty(mem);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatASmallerFreedBlockIsNotUsedForANewAllocationButANewContiguousBlockIsUsed)
{
  auto mem1 = sut.allocate(SmallChunkSize);
  auto mem2 = sut.allocate(SmallChunkSize * 2);

  auto ptrOf1stLocation = mem1.ptr;
  deallocateAndCheckBlockIsThenEmpty(mem1);

  auto mem3 = sut.allocate(SmallChunkSize * 2);

  ASSERT_NE(ptrOf1stLocation, mem3.ptr);
  EXPECT_EQ(static_cast<char*>(mem3.ptr), static_cast<char*>(mem2.ptr) + SmallChunkSize * 2);

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
  auto mem = sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, 0));

  EXPECT_EQ(origMem, mem);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), mem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandByNBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, 4));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_GE(mem.length, origMem.length + 4);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandingOfABlockIsWithinAUsedAreaThatFitsIntoTheGapIsSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*2>(sut).withAUsedPatternOf("1100'0110").build();

  auto mem = sut.allocate(SmallChunkSize*4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, SmallChunkSize*2));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_GE(mem.length, origMem.length + SmallChunkSize);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandingOfABlockIsWithinAUsedAreaThatDoesNotFitsIntoTheGapIsNotSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*2>(sut).withAUsedPatternOf("1100'0110").build();

  auto mem = sut.allocate(SmallChunkSize*6);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_FALSE(sut.expand(mem, SmallChunkSize));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(mem.length, origMem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandingOfABlockBeyondTheSizeOfASingleControlRegsiterIsSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*4>(sut).withAUsedPatternOf("1000'0000'0000'0000").build();

  auto mem = sut.allocate(SmallChunkSize*4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, SmallChunkSize * 64));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 64), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatExpandingOfABlockBeyondTheSizeOfATwoControlRegisterIsSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*4>(sut).withAUsedPatternOf("1111'1111'1111'1100").build();

  auto mem = sut.allocate(SmallChunkSize*4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.expand(mem, SmallChunkSize*128));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 128), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}


TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocatrByZeroBytesOfAnEmptyBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(0);

  EXPECT_TRUE(sut.reallocate(mem, 0));

  EXPECT_EQ(nullptr, mem.ptr);
  EXPECT_EQ(0, mem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocateByZeroBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, SmallChunkSize));

  EXPECT_EQ(origMem, mem);
  EXPECT_EQ(SmallChunkSize, mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), mem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocateByNBytesOfAFilledBlockReturnsSuccessAndDoesNotChangeTheProvidedBlock)
{
  auto mem = sut.allocate(SmallChunkSize);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, SmallChunkSize + 4));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize + SmallChunkSize, mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocatrOfABlockIsWithinAUsedAreaThatFitsIntoTheGapIsSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*2>(sut).withAUsedPatternOf("1100'0110").build();

  auto mem = sut.allocate(SmallChunkSize*4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, SmallChunkSize*(4+2)));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize*(4+2), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocateOfABlockIsWithinAUsedAreaThatDoesNotFitsIntoTheGapIsSuccessfulWithANewLocations) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*2>(sut).withAUsedPatternOf("1100'0110").build();

  auto mem = sut.allocate(SmallChunkSize*6);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, SmallChunkSize*(6+1)));

  EXPECT_NE(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize*(6+1), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocateOfABlockBeyondTheSizeOfASingleControlRegsiterIsSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*4>(sut).withAUsedPatternOf("1000'0000'0000'0000").build();

  auto mem = sut.allocate(SmallChunkSize*4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, SmallChunkSize * (4+64)));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 64), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}

TEST_F(SharedHeapWithSmallAllocationsTest, ThatReallocateOfABlockBeyondTheSizeOfATwoControlRegisterIsSuccessful) {
  auto usedMem = UsedMemGenerator<AllocatorBaseTest::allocator, 
    SmallChunkSize*4>(sut).withAUsedPatternOf("1111'1111'1111'1100").build();

  auto mem = sut.allocate(SmallChunkSize*4);
  auto origMem = mem;
  fillBlockWithReferenceData<int>(mem);

  EXPECT_TRUE(sut.reallocate(mem, SmallChunkSize*(128 + 4)));

  EXPECT_EQ(origMem.ptr, mem.ptr);
  EXPECT_EQ(SmallChunkSize * (4 + 128), mem.length);
  EXPECT_MEM_EQ(mem.ptr, (void*)ReferenceData.data(), origMem.length);
}





class SharedHeapWithLargeAllocationsTest : public 
  AllocatorBaseTest<ALB::SharedHeap<ALB::Mallocator, 512, 8>>
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

  TestWorker<AllocatorUnderTest> testWorker(sut, 128);
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
  
  TestWorkerCollector<
    AllocatorUnderTest, 
    2, 
    TestWorker<AllocatorUnderTest>,
    TestParams> testCollector(sut, maxAllocatedBytes);

  testCollector.check();

  auto allDeallocatedCheck = sut.allocate(512*64);
  EXPECT_TRUE((bool)allDeallocatedCheck);
}


TEST_F(SharedHeapTreatetWithThreadsTest, BruteForceTestWith4ThreadsRunningHoldingMultipleAllocations)
{
  const size_t NumberOfChunks = 1024;
  const size_t BlockSize = 64;
  const size_t NumberOfThread = 2;

  typedef AffixGuard<unsigned, 0xbaadf00d> PrefixGuard;
  typedef AffixGuard<unsigned, 0xf000baaa> SufixGuard;

  typedef ALB::AffixAllocator<
    ALB::SharedHeap<ALB::Mallocator, NumberOfChunks, BlockSize>, PrefixGuard, SufixGuard> AllocatorUnderTest;

  AllocatorUnderTest sut;

  typedef std::array<unsigned char, NumberOfThread> TestParams;
  //TestParams maxAllocatedBytes = {127, 131, 165, 129};
  TestParams maxAllocatedBytes = {127, 131};

  TestWorkerCollector<
    AllocatorUnderTest, 
    NumberOfThread, 
    MultipleAllocationsTester<AllocatorUnderTest>,
    TestParams> testCollector(sut, maxAllocatedBytes);

  testCollector.check();

  // At the end, we check that all memory was released. The easiest way to check this
  // is by trying to allocate everything
  auto allDeallocatedCheck = sut.allocate(
    NumberOfChunks * BlockSize 
    - AllocatorUnderTest::prefix_size - AllocatorUnderTest::sufix_size);

  EXPECT_TRUE(allDeallocatedCheck);
}

