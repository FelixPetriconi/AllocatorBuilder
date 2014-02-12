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
#include <limits>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace ALB
{
  /**
   * The SharedHeap implements a classic heap with a pre-allocated size of NumberOfBlocks * BlockSize
   * It has a overhead of one bit per block and linear complexity for allocation and deallocation
   * operations.
   * It is thread safe, except the moment of instantiation.
   * As far as possible only a shared lock + an atomic operation is used during the memory operations
   *
   * TODO: * Dynastic for BlockSize
   *       * implement expand
   *       * improve reallocate
   *       * Handle more relaxed the NumberOfBlocks
   */
  template <class Allocator, size_t NumberOfBlocks, size_t BlockSize>
  class SharedHeap {
    typedef Allocator allocator;

    static const size_t number_of_blocks = NumberOfBlocks;
    static const size_t block_size = BlockSize;

    static_assert(NumberOfBlocks % (sizeof(uint64_t) * 8) == 0, "NumberOfBlocks is not correct aligned!");
    static const size_t ControlSize = NumberOfBlocks / (sizeof(uint64_t) * 8);
  
    uint64_t all_set;
    uint64_t all_zero;
    Block _buffer;

    std::atomic<uint64_t> _control[ControlSize]; // bit field where 0 means used and 1 means free block

    boost::shared_mutex _mutex;
    Allocator _allocator;


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

                if (_control[controlIndex].compare_exchange_strong(currentControlRegister, newControlRegister)) {
                  size_t ptrOffset = (controlIndex * 64 + i ) * BlockSize;
                  return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * BlockSize);
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

        if (freeChunk->compare_exchange_strong(all_set, all_zero)) {
          size_t ptrOffset = ( (freeChunk - std::begin(_control)) * 64) * BlockSize;
          return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * BlockSize);
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
        p->compare_exchange_strong(all_set, all_zero);
        ++p;
      }

      size_t ptrOffset = ( (freeFirstChunk - std::begin(_control)) * 64) * BlockSize;
      return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * BlockSize);
    }

    Block allocateWithRegisterOverlap(size_t numberOfBlocks) {
      // This branch works on multiple chunks at the same time and so a real lock is necessary.
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      // search for free area
      static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t), "Current assumption that std::atomic has no overhead on integral types is not fullfilled!");

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
        int blocksToMark = static_cast<int>(numberOfBlocks);
        p = chunkStart;
        while (blocksToMark > 0) {
          if (blocksToMark > 8) {
            *p = 0;
            p++;
            blocksToMark -= 8;
          }
          else {
            unsigned char mask = ((1 << blocksToMark) - 1) ^ 0xff;
            *p &= mask;
            blocksToMark = 0;
          }
        }
        size_t ptrOffset = (chunkStart - reinterpret_cast<unsigned char*>(&_control[0])) * 8 * BlockSize;
        return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * BlockSize);
      }

      return Block();
    }


    void deallocateWithinSingleControlRegister(Block& b, size_t numberOfBlocks, size_t freeBlockFieldIndex, size_t subIndex) {
      uint64_t mask = (1uLL << numberOfBlocks) - 1;
      mask <<= subIndex;

      uint64_t currentChunk, newChunk;
      do {
        currentChunk = _control[freeBlockFieldIndex].load();
        assert( (currentChunk & mask) == 0);

        newChunk = currentChunk | mask;
        boost::shared_lock< boost::shared_mutex > guard(_mutex);
      }
      while (!_control[freeBlockFieldIndex].compare_exchange_strong(currentChunk, newChunk));
      b.reset();
      return;
    }

    void deallocateOfACompleteControlRegister(Block& b, size_t freeBlockFieldIndex) {
      boost::shared_lock< boost::shared_mutex > guard(_mutex);

      if (!_control[freeBlockFieldIndex].compare_exchange_strong(all_zero, all_set)) {
        assert(!"Heap Corruption");
      }
      b.reset();
    }

    void deallocateForMultipleCompleteControlRegister(Block& b, size_t numberOfBlocks, size_t freeBlockFieldIndex) {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      std::fill(std::begin(_control) + freeBlockFieldIndex, 
        std::begin(_control) + freeBlockFieldIndex + numberOfBlocks / 64, static_cast<uint64_t>(-1));

      b.reset();
    }

    void deallocateWithControlRegisterOverlap(Block& b, size_t numberOfBlocks, size_t freeBlockFieldIndex, size_t subIndex) {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      unsigned char* p = reinterpret_cast<unsigned char*>(&_control[0]) 
        + freeBlockFieldIndex * sizeof(uint64_t) + subIndex / 8;

      int blocksToMark = static_cast<int>(numberOfBlocks);
      while (blocksToMark > 0) {
        if (blocksToMark > 8) {
          *p = 0xFF;
          p++;
          blocksToMark -= 8;
        }
        else {
          unsigned char mask = ((1 << blocksToMark) - 1);
          *p |= mask;
          blocksToMark = 0;
        }
      }
      b.reset();
    }
  public:

    SharedHeap()
      : all_set(static_cast<uint64_t>(-1))
      , all_zero(0) {
      _buffer = _allocator.allocate( BlockSize * NumberOfBlocks );
      deallocateAll();
    }


    ~SharedHeap() {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);
      _allocator.deallocate(_buffer);
    }

    bool owns(Block b) {
      return _buffer.ptr <= b.ptr && b.ptr < (static_cast<char*>(_buffer.ptr) + _buffer.length);
    }

    Block allocate(size_t n) {
      if (n == 0) {
        return Block();
      }

      if ( n > BlockSize * NumberOfBlocks) { // The heap cannot handle such a big request
        return Block();
      }

      size_t numberOfAlignedBytes = Helper::roundToAlignment<BlockSize>(n);
      size_t numberOfBlocks = numberOfAlignedBytes / BlockSize;
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
      if (b.length == 0) {
        assert(b.ptr == nullptr);
        return;
      }

      assert(b.length % BlockSize == 0);
      assert(owns(b));

      size_t numberOfBlocks = b.length / BlockSize;
      int blockIndex = static_cast<int>( (static_cast<char*>(b.ptr) - static_cast<char*>(_buffer.ptr)) / BlockSize );
      int freeBlockFieldIndex = blockIndex / 64;
      int subIndex = blockIndex % 64;
      
      //printf("Used Block %d in thread %d\n", blockIndex, std::this_thread::get_id());
      if (numberOfBlocks < 64) {
        deallocateWithinSingleControlRegister(b, numberOfBlocks, freeBlockFieldIndex, subIndex);
        if (!b) {
          return;
        }

        // Handle cases with register overlap
      }
      else if (numberOfBlocks == 64) {
        deallocateOfACompleteControlRegister(b, freeBlockFieldIndex);
        if (!b) {
          return;
        }
      }
      else if ( (numberOfBlocks % 64) == 0 ) {
        deallocateForMultipleCompleteControlRegister(b, numberOfBlocks, freeBlockFieldIndex);
        if (!b) {
          return;
        }
      }
      else {
        deallocateWithControlRegisterOverlap(b, numberOfBlocks, freeBlockFieldIndex, subIndex);
        if (!b) {
          return;
        }
      }
    }

    void deallocateAll() {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      std::fill(std::begin(_control), std::end(_control), static_cast<uint64_t>(-1));
    }

    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<SharedHeap>::isHandledDefault(*this, b, n)){
        return true;
      }

      if (n < b.length) {
        // handle insito
      }
     
      // most simple version for the beginning
      return Helper::reallocateWithCopy(*this, *this, b, n);
      // case that next to the block is in the same chunk remaining space, doable without lock
      // case that less memory is needed within the same chunk, doable without lock
      // all others need lock
    }
  };

}
