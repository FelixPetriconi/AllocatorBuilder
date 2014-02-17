///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
///////////////////////////////////////////////////////////////////
#pragma once

#include "ALBAllocatorBase.h"

#include <atomic>
#include <boost/thread.hpp>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define CAS(ATOMIC, EXPECT, VALUE) \
  ATOMIC.compare_exchange_strong(EXPECT, VALUE)

#define CAS_P(ATOMIC, EXPECT, VALUE) \
  ATOMIC->compare_exchange_strong(EXPECT, VALUE)

namespace ALB
{
  /**
   * The SharedHeap implements a classic heap with a pre-allocated size of NumberOfChunks * ChunkSize
   * It has a overhead of one bit per block and linear complexity for allocation and deallocation
   * operations.
   * It is thread safe, except the moment of instantiation.
   * As far as possible only a shared lock + an atomic operation is used during the memory operations
   *
   * TODO: * Dynastic for ChunkSize
   *       * implement expand
   *       * improve reallocate
   *       * Handle more relaxed the NumberOfChunks
   */
  template <class Allocator, size_t NumberOfChunks, size_t ChunkSize>
  class SharedHeap {
    static_assert(NumberOfChunks % (sizeof(uint64_t) * 8) == 0, "NumberOfChunks is not correct aligned!");
    static const size_t ControlSize = NumberOfChunks / (sizeof(uint64_t) * 8);
  
    uint64_t all_set;
    uint64_t all_zero;
    Block _buffer;

    std::atomic<uint64_t> _control[ControlSize]; // bit field where 0 means used and 1 means free block

    boost::shared_mutex _mutex;
    Allocator _allocator;
    
    struct BlockContext {
      BlockContext(int ri, int si, int uc) : registerIndex(ri), subIndex(si), usedChunks(uc) {}
      int registerIndex, subIndex, usedChunks;
    };
    
    BlockContext blockToContext(const Block& b) {
      const int blockIndex = static_cast<int>( (static_cast<char*>(b.ptr) - static_cast<char*>(_buffer.ptr)) / ChunkSize );
      return BlockContext(blockIndex / 64, blockIndex % 64, static_cast<int>(b.length / ChunkSize));
    }

    template <bool Used>
    uint64_t setUsed(const uint64_t& currentRegister, const uint64_t& mask);

    template <>
    uint64_t setUsed<false>(const uint64_t& currentRegister, const uint64_t& mask) {
      return currentRegister & (mask ^ all_set);
    }
    template <>
    uint64_t setUsed<true>(const uint64_t& currentRegister, const uint64_t& mask) {
      return currentRegister | mask;
    }


    template <bool Used>
    bool testAndSetWithinSingleRegister(const BlockContext& context) {
      assert(context.subIndex + context.usedChunks <= 64);

      uint64_t mask = (context.usedChunks == 64)? all_set : ( ((1uLL << context.usedChunks) - 1) << context.subIndex);
      uint64_t currentRegister, newRegister;
      do {
        currentRegister = _control[context.registerIndex].load();
        if ( (currentRegister & mask) != mask) {
          return false;
        }
        newRegister = setUsed<Used>(currentRegister, mask);
        boost::shared_lock< boost::shared_mutex > guard(_mutex);
      } while (!CAS(_control[context.registerIndex], currentRegister, newRegister));
      return true;
    }

    template <bool Used>
    void setOverMultipleRegistersWithoutLock(const BlockContext& context) {
      size_t chunksToTest = context.usedChunks;
      size_t subIndexStart = context.subIndex;
      size_t registerIndex = context.registerIndex;
      do {
        size_t mask;
        if (subIndexStart > 0)
          mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
        else
          mask = (chunksToTest >= 64)? all_set : ((1uLL << chunksToTest) - 1);

        assert(registerIndex < ControlSize);

        auto newRegister = setUsed<Used>(_control[registerIndex].load(), mask);
        _control[registerIndex] = newRegister;

        if (subIndexStart + chunksToTest > 64) {
          chunksToTest = subIndexStart + chunksToTest - 64;
          subIndexStart = 0;
        }
        else {
          chunksToTest = 0;            
        }
        registerIndex++;
      }
      while (chunksToTest > 0);
    }

    template <bool Used>
    bool testAndSetOverMultipleRegisters(const BlockContext& context) {
      static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t), 
        "Current assumption that std::atomic has no overhead on integral types is not fullfilled!");

      // This branch works on multiple chunks at the same time and so a real lock is necessary.
      size_t chunksToTest = context.usedChunks;
      size_t subIndexStart = context.subIndex;
      size_t registerIndex = context.registerIndex;

      boost::unique_lock< boost::shared_mutex > guard(_mutex);
      do {
        uint64_t mask;
        if (subIndexStart > 0)
          mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
        else
          mask = (chunksToTest >= 64)? all_set : ((1uLL << chunksToTest) - 1);

        auto currentRegister = _control[registerIndex].load();
        
        if ( (currentRegister & mask) != mask) {
          return false;
        }
        
        if (subIndexStart + chunksToTest > 64) {
          chunksToTest = subIndexStart + chunksToTest - 64;
          subIndexStart = 0;
        }
        else {
          chunksToTest = 0;            
        }
        registerIndex++;
        if (registerIndex > ControlSize) {
          return false;
        }
      }
      while (chunksToTest > 0); 
      
      setOverMultipleRegistersWithoutLock<Used>(context);

      return true;
    }


    template <bool Used>
    void setWithinSingleRegister(const BlockContext& context) {
      assert(context.subIndex + context.usedChunks <= 64);

      uint64_t mask = (context.usedChunks == 64)? all_set : ( ((1uLL << context.usedChunks) - 1) << context.subIndex);
      uint64_t currentRegister, newRegister;
      do {
        currentRegister = _control[context.registerIndex].load();
        newRegister = setUsed<Used>(currentRegister, mask);
        boost::shared_lock< boost::shared_mutex > guard(_mutex);
      } while (!CAS(_control[context.registerIndex], currentRegister, newRegister));
    }


    Block allocateWithinASingleControlRegister(size_t numberOfBlocks)
    {
      // we must assume that we may find a free location, but that it is later already used during the set operation
      do {
        // first we have to look for at least one free block
        int controlIndex = 0;
        while (controlIndex < ControlSize) {
          auto currentControlRegister = _control[controlIndex].load();

          if (currentControlRegister != 0) { // == 0 means that all blocks are in use and no need to search further
            uint64_t mask = (numberOfBlocks == 64)? all_set : ((1uLL << numberOfBlocks) - 1);

            int i = 0;
            // Search for numberOfBlock bits that are set to one
            while (i <= 64 - numberOfBlocks) {
              if ((currentControlRegister & mask) == mask) {
                auto newControlRegister = currentControlRegister & (mask ^ all_set);

                boost::shared_lock< boost::shared_mutex > guard(_mutex);

                if (CAS(_control[controlIndex], currentControlRegister, newControlRegister)) {
                  size_t ptrOffset = (controlIndex * 64 + i ) * ChunkSize;
                  return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * ChunkSize);
                }
              }
              i++;
              mask <<= 1;
            };
          }
          controlIndex++;
        }
        if (controlIndex == ControlSize) {
          return Block();
        }
      } while (true);
    }

    Block allocateWithinCompleteControlRegister(size_t numberOfBlocks) {
      // we must assume that we may find a free location, but that it is later already used during the CAS set operation
      do {
        // first we have to look for at least full free block
        auto freeChunk = std::find_if(std::begin(_control), std::end(_control), 
          [this](const std::atomic<uint64_t>& v) { return v.load() == all_set; });

        if (freeChunk == std::end(_control)) {
          return Block();
        }

        boost::shared_lock< boost::shared_mutex > guard(_mutex);

        if (CAS_P(freeChunk, all_set, all_zero)) {
          size_t ptrOffset = ( (freeChunk - std::begin(_control)) * 64) * ChunkSize;
          return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * ChunkSize);
        }
      } while (true);
    }

    Block allocateMultipleCompleteControlRegisters(size_t numberOfBlocks) {
      // This branch works on multiple chunks at the same time and so a real lock is necessary.
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      const int neededChunks = static_cast<int>(numberOfBlocks / 64);
      auto freeFirstChunk = std::search_n(std::begin(_control), std::end(_control), 
        neededChunks, all_set, [](const std::atomic<uint64_t>&v, const uint64_t& p) { return v.load() == p;});

      if (freeFirstChunk == std::end(_control)) {
        return Block();
      }
      auto p = freeFirstChunk;
      while (p < freeFirstChunk + neededChunks) {
        CAS_P(p, all_set, all_zero);
        ++p;
      }

      size_t ptrOffset = ( (freeFirstChunk - std::begin(_control)) * 64) * ChunkSize;
      return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * ChunkSize);
    }

    Block allocateWithRegisterOverlap(size_t numberOfBlocks) {
      // search for free area
      static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t), 
        "Current assumption that std::atomic has no overhead on integral types is not fullfilled!");

      // This branch works on multiple chunks at the same time and so a real lock is necessary.
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      unsigned char* p = reinterpret_cast<unsigned char*>(&_control[0]);
      const unsigned char* lastp = reinterpret_cast<unsigned char*>(&_control[0]) + ControlSize * sizeof(uint64_t);

      int freeBlocksCount(0);
      unsigned char* chunkStart = nullptr;
      while (p < lastp) {
        if (*p == 0xff) { // free
          if (!chunkStart) {
            chunkStart = p;
          }

          freeBlocksCount += 8;
          if (freeBlocksCount >= numberOfBlocks) {
            break;
          }
        }
        else {
          freeBlocksCount = 0;
          chunkStart = nullptr;
        }
        p++;
      };

      if (p != lastp && freeBlocksCount >= numberOfBlocks) {
        size_t ptrOffset = (chunkStart - reinterpret_cast<unsigned char*>(&_control[0])) * 8 * ChunkSize;
        Block result(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * ChunkSize);
        setOverMultipleRegistersWithoutLock<false>(blockToContext(result));
        return result;
      }

      return Block();
    }


    void deallocateForMultipleCompleteControlRegister(const BlockContext& context) {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      std::fill(std::begin(_control) + context.registerIndex, 
        std::begin(_control) + context.registerIndex + context.usedChunks/ 64, static_cast<uint64_t>(-1));
    }

    void deallocateWithControlRegisterOverlap(const BlockContext& context) {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);
      setOverMultipleRegistersWithoutLock<true>(context);
    }

  public:
    typedef Allocator allocator;
    static const size_t number_of_chunks = NumberOfChunks;
    static const size_t chunk_size = ChunkSize;
    
    SharedHeap()
      : all_set(static_cast<uint64_t>(-1))
      , all_zero(0) {
      _buffer = _allocator.allocate( ChunkSize * NumberOfChunks );
      deallocateAll();
    }


    ~SharedHeap() {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);
      _allocator.deallocate(_buffer);
    }

    bool owns(const Block& b) const {
      return b && _buffer.ptr <= b.ptr && b.ptr < (static_cast<char*>(_buffer.ptr) + _buffer.length);
    }

    Block allocate(size_t n) {
      if (n == 0) {
        return Block();
      }

      if ( n > ChunkSize * NumberOfChunks) { // The heap cannot handle such a big request
        return Block();
      }

      size_t numberOfAlignedBytes = Helper::roundToAlignment<ChunkSize>(n);
      size_t numberOfBlocks = numberOfAlignedBytes / ChunkSize;
      numberOfBlocks = std::max(1uLL, numberOfBlocks);

      if (numberOfBlocks < 64)
      {
        auto result = allocateWithinASingleControlRegister(numberOfBlocks);
        if (result) {
          return result;
        }
        // Try to allocate over controlRegisterEdges

      }
      else if (numberOfBlocks == 64) {
        auto result = allocateWithinCompleteControlRegister(numberOfBlocks);
        if (result) {
          return result;
        }
        // Try to allocate over controlRegisterEdges
      }
      else if ( (numberOfBlocks % 64) == 0) {
        auto result = allocateMultipleCompleteControlRegisters(numberOfBlocks);
        if (result) {
          return result;
        }
        // Try to allocate over controlRegisterEdges
      }
      return allocateWithRegisterOverlap(numberOfBlocks);
    }


    void deallocate(Block& b) {
      if (!b) {
        return;
      }

      assert(b.length % ChunkSize == 0);
      assert(owns(b));

      const auto context = blockToContext(b);
            
      //printf("Used Block %d in thread %d\n", blockIndex, std::this_thread::get_id());
      if (context.subIndex + context.usedChunks <= 64) {
        setWithinSingleRegister<true>(context);
        // TODO
        // Handle cases with register overlap
      }
      else if ( (context.usedChunks % 64) == 0 ) {
        deallocateForMultipleCompleteControlRegister(context);
      }
      else {
        deallocateWithControlRegisterOverlap(context);
      }
      b.reset();
    }

    void deallocateAll() {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      std::fill(std::begin(_control), std::end(_control), static_cast<uint64_t>(-1));
    }
    
    // TODO
    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<SharedHeap>::isHandledDefault(*this, b, n)){
        return true;
      }

      const size_t numberOfBlocks = b.length / ChunkSize;
      const size_t numberOfAlignedBytes = Helper::roundToAlignment<ChunkSize>(n);
      const size_t numberOfNewNeededBlocks = numberOfAlignedBytes / ChunkSize;
      
      if (numberOfBlocks == numberOfNewNeededBlocks) {
        return true;
      }

      int blockIndex = static_cast<int>( (static_cast<char*>(b.ptr) - static_cast<char*>(_buffer.ptr)) / ChunkSize );
      int freeBlockFieldIndex = blockIndex / 64;
      int subIndex = blockIndex % 64;

      // no lock is needed here, because the necessary operations are handled inside the method under lock
      // most simple version for the beginning
      return Helper::reallocateWithCopy(*this, *this, b, n);
      // case that next to the block is in the same chunk remaining space, doable without lock
      // case that less memory is needed within the same chunk, doable without lock
      // all others need lock
    }

    // TODO
    bool expand(Block& b, size_t delta) {
      if (delta == 0) {
        return true;
      }

      const auto context = blockToContext(b);
      const int numberOfAdditionalNeededBlocks = 
        static_cast<int>(Helper::roundToAlignment<ChunkSize>(delta) / ChunkSize);
      
      if (context.subIndex + context.usedChunks + numberOfAdditionalNeededBlocks <= 64) {
        if (testAndSetWithinSingleRegister<false>(BlockContext(context.registerIndex, 
                                                              context.subIndex + context.usedChunks, 
                                                              numberOfAdditionalNeededBlocks)) ) {
          b.length += numberOfAdditionalNeededBlocks * ChunkSize;
          return true;
        }
        else {
          return false;
        }
      }
      else {
        if (testAndSetOverMultipleRegisters<false>(BlockContext(context.registerIndex,
                                                               context.subIndex + context.usedChunks,
                                                               numberOfAdditionalNeededBlocks)) ) {
          b.length += numberOfAdditionalNeededBlocks * ChunkSize;
          return true;
        }
      }
      return false;
    }
  };

}
