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
#include "ALBSharedHelpers.h"

#include <atomic>
#include <boost/thread.hpp>
#include <boost/assert.hpp>

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
   * The SharedHeap implements a classic heap with a pre-allocated size of _numberOfChunks.value() * _chunkSize.value()
   * It has a overhead of one bit per block and linear complexity for allocation and deallocation
   * operations.
   * It is thread safe, except the moment of instantiation.
   * As far as possible only a shared lock + an atomic operation is used during the memory operations
   * \ingroup group_allocators group_shared
   */
  template <class Allocator, size_t NumberOfChunks, size_t ChunkSize>
  class SharedHeap {
    const uint64_t all_set;
    const uint64_t all_zero;

    Helper::Dynastic<(NumberOfChunks == DynamicSetSize? 0 : NumberOfChunks), 0> _numberOfChunks;
    Helper::Dynastic<(ChunkSize == DynamicSetSize? 0 : ChunkSize), 0> _chunkSize;
    
    Block _buffer;
    Block _controlBuffer;

    // bit field where 0 means used and 1 means free block
    std::atomic<uint64_t>* _control; 
    size_t _controlSize;

    boost::shared_mutex _mutex;
    Allocator _allocator;

  public:
    static const bool supports_truncated_deallocation = true;
    typename typedef Allocator allocator;

    
    SharedHeap()
      : all_set(static_cast<uint64_t>(-1))
      , all_zero(0) {

      init();
    }

    SharedHeap(size_t numberOfChunks, size_t chunkSize)
      : all_set(static_cast<uint64_t>(-1))
      , all_zero(0) {

      _numberOfChunks.value( Helper::roundToAlignment(64, numberOfChunks) );
      _chunkSize.value( Helper::roundToAlignment(4, chunkSize) );
      
      init();
    }

    size_t number_of_chunk() const {
      return _numberOfChunks.value();
    }

    size_t chunk_size() const {
      return _chunkSize.value();
    }


    ~SharedHeap() {
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      _allocator.deallocate(_controlBuffer);
      _allocator.deallocate(_buffer);
      
      _control = nullptr;
    }

    bool owns(const Block& b) const {
      return b && _buffer.ptr <= b.ptr && b.ptr < (static_cast<char*>(_buffer.ptr) + _buffer.length);
    }

    Block allocate(size_t n) {
      if (n == 0) {
        return Block();
      }

      if ( n > _chunkSize.value() * _numberOfChunks.value()) { // The heap cannot handle such a big request
        return Block();
      }

      size_t numberOfAlignedBytes = Helper::roundToAlignment(_chunkSize.value(), n);
      size_t numberOfBlocks = numberOfAlignedBytes / _chunkSize.value();
      numberOfBlocks = std::max(1uLL, numberOfBlocks);

      if (numberOfBlocks < 64)
      {
        auto result = allocateWithinASingleControlRegister(numberOfBlocks);
        if (result) {
          return result;
        }
      }
      else if (numberOfBlocks == 64) {
        auto result = allocateWithinCompleteControlRegister(numberOfBlocks);
        if (result) {
          return result;
        }
      }
      else if ( (numberOfBlocks % 64) == 0) {
        auto result = allocateMultipleCompleteControlRegisters(numberOfBlocks);
        if (result) {
          return result;
        }
      }
      return allocateWithRegisterOverlap(numberOfBlocks);
    }


    void deallocate(Block& b) {
      if (!b) {
        return;
      }    
      BOOST_ASSERT_MSG(owns(b), "It is not wise to let me deallocate a foreign Block!");
      if (!owns(b)) {
        return;
      }

      const auto context = blockToContext(b);
            
      //printf("Used Block %d in thread %d\n", blockIndex, std::this_thread::get_id());
      if (context.subIndex + context.usedChunks <= 64) {
        setWithinSingleRegister<SharedHelpers::SharedLock, true>(context);
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

      std::fill(_control, _control + _controlSize, static_cast<uint64_t>(-1));
    }
    
    bool reallocate(Block& b, size_t n) {
      if (Helper::Reallocator<SharedHeap>::isHandledDefault(*this, b, n)){
        return true;
      }

      const int numberOfBlocks = static_cast<int>(b.length / _chunkSize.value());
      const int numberOfNewNeededBlocks = static_cast<int>(
        Helper::roundToAlignment(_chunkSize.value(), n) / _chunkSize.value());
      
      if (numberOfBlocks == numberOfNewNeededBlocks) {
        return true;
      }
      if (b.length > n) {
        auto context = blockToContext(b);
        if (context.subIndex + context.usedChunks <= 64) {
          setWithinSingleRegister<SharedHelpers::SharedLock, true>(BlockContext(context.registerIndex, 
            context.subIndex + numberOfNewNeededBlocks, 
            context.usedChunks - numberOfNewNeededBlocks));
        } 
        else {
          deallocateWithControlRegisterOverlap(BlockContext(context.registerIndex, 
            context.subIndex + numberOfNewNeededBlocks, 
            context.usedChunks - numberOfNewNeededBlocks));
        }
        b.length = numberOfNewNeededBlocks * _chunkSize.value();
        return true;
      }
      return Helper::reallocateWithCopy(*this, *this, b, n);
    }


    bool expand(Block& b, size_t delta) {
      if (delta == 0) {
        return true;
      }

      const auto context = blockToContext(b);
      const int numberOfAdditionalNeededBlocks = 
        static_cast<int>(Helper::roundToAlignment(_chunkSize.value(), delta) / _chunkSize.value());
      
      if (context.subIndex + context.usedChunks + numberOfAdditionalNeededBlocks <= 64) {
        if (testAndSetWithinSingleRegister<false>(BlockContext(context.registerIndex, 
                                                              context.subIndex + context.usedChunks, 
                                                              numberOfAdditionalNeededBlocks)) ) {
          b.length += numberOfAdditionalNeededBlocks * _chunkSize.value();
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
          b.length += numberOfAdditionalNeededBlocks * _chunkSize.value();
          return true;
        }
      }
      return false;
    }


  private:
    void init() {
      _controlSize = _numberOfChunks.value()/64;
      _controlBuffer = _allocator.allocate(sizeof(std::atomic<uint64_t>) * _controlSize);
      BOOST_ASSERT((bool)_controlBuffer);

      _control = static_cast<std::atomic<uint64_t>*>(_controlBuffer.ptr);
      new (_control) std::atomic<uint64_t>[_controlSize]();

      _buffer = _allocator.allocate( _chunkSize.value() * _numberOfChunks.value() );
      BOOST_ASSERT((bool)_buffer);

      deallocateAll();
    }

    struct BlockContext {
      BlockContext(int ri, int si, int uc) : registerIndex(ri), subIndex(si), usedChunks(uc) {}
      int registerIndex, subIndex, usedChunks;
    };

    BlockContext blockToContext(const Block& b) {
      const int blockIndex = static_cast<int>( (static_cast<char*>(b.ptr) - static_cast<char*>(_buffer.ptr)) / _chunkSize.value() );
      return BlockContext(blockIndex / 64, blockIndex % 64, static_cast<int>(b.length / _chunkSize.value()));
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
      BOOST_ASSERT(context.subIndex + context.usedChunks <= 64);

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

    template <class LockPolicy, bool Used>
    void setOverMultipleRegisters(const BlockContext& context) {
      size_t chunksToTest = context.usedChunks;
      size_t subIndexStart = context.subIndex;
      size_t registerIndex = context.registerIndex;
      do {
        size_t mask;
        if (subIndexStart > 0)
          mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
        else
          mask = (chunksToTest >= 64)? all_set : ((1uLL << chunksToTest) - 1);

        BOOST_ASSERT(registerIndex < _controlSize);

        uint64_t currentRegister, newRegister;
        do {
          currentRegister = _control[registerIndex].load();
          newRegister = setUsed<Used>(currentRegister, mask);
          LockPolicy guard(_mutex);            
        } while (!CAS(_control[registerIndex], currentRegister, newRegister));

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
        if (registerIndex > _controlSize) {
          return false;
        }
      }
      while (chunksToTest > 0); 

      setOverMultipleRegisters<SharedHelpers::NullLock, Used>(context);

      return true;
    }


    template <class LockPolicy, bool Used>
    void setWithinSingleRegister(const BlockContext& context) {
      BOOST_ASSERT(context.subIndex + context.usedChunks <= 64);

      uint64_t mask = (context.usedChunks == 64)? all_set : 
        ( ((1uLL << context.usedChunks) - 1) << context.subIndex);

      uint64_t currentRegister, newRegister;
      do {
        currentRegister = _control[context.registerIndex].load();
        newRegister = setUsed<Used>(currentRegister, mask);
        LockPolicy guard(_mutex);
      } while (!CAS(_control[context.registerIndex], currentRegister, newRegister));
    }


    Block allocateWithinASingleControlRegister(size_t numberOfBlocks)
    {
      // we must assume that we may find a free location, but that it is later already used during the set operation
      do {
        // first we have to look for at least one free block
        int controlIndex = 0;
        while (controlIndex < _controlSize) {
          auto currentControlRegister = _control[controlIndex].load();

          if (currentControlRegister != 0) { // == 0 means that all blocks are in use and no need to search further
            uint64_t mask = (numberOfBlocks == 64)? all_set : ((1uLL << numberOfBlocks) - 1);

            int i = 0;
            // Search for numberOfBlock bits that are set to one
            while (i <= 64 - numberOfBlocks) {
              if ((currentControlRegister & mask) == mask) {
                auto newControlRegister = setUsed<false>(currentControlRegister, mask);

                boost::shared_lock< boost::shared_mutex > guard(_mutex);

                if (CAS(_control[controlIndex], currentControlRegister, newControlRegister)) {
                  size_t ptrOffset = (controlIndex * 64 + i ) * _chunkSize.value();
                  return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * _chunkSize.value());
                }
              }
              i++;
              mask <<= 1;
            };
          }
          controlIndex++;
        }
        if (controlIndex == _controlSize) {
          return Block();
        }
      } while (true);
    }

    Block allocateWithinCompleteControlRegister(size_t numberOfBlocks) {
      // we must assume that we may find a free location, but that it is later 
      // already used during the CAS set operation
      do {
        // first we have to look for at least full free block
        auto freeChunk = std::find_if(_control, _control + _controlSize, 
          [this](const std::atomic<uint64_t>& v) { return v.load() == all_set; });

        if (freeChunk == _control + _controlSize) {
          return Block();
        }

        boost::shared_lock< boost::shared_mutex > guard(_mutex);

        if (CAS_P(freeChunk, const_cast<uint64_t&>(all_set), all_zero)) {
          size_t ptrOffset = ( (freeChunk - _control) * 64) * _chunkSize.value();
          return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * _chunkSize.value());
        }
      } while (true);
    }

    Block allocateMultipleCompleteControlRegisters(size_t numberOfBlocks) {
      // This branch works on multiple chunks at the same time and so a real lock is necessary.
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

      const int neededChunks = static_cast<int>(numberOfBlocks / 64);
      auto freeFirstChunk = std::search_n(_control, _control + _controlSize, 
        neededChunks, 
        all_set, 
        [](const std::atomic<uint64_t>&v, const uint64_t& p) { 
          return v.load() == p;
        }
      );

      if (freeFirstChunk == _control + _controlSize) {
        return Block();
      }
      auto p = freeFirstChunk;
      while (p < freeFirstChunk + neededChunks) {
        CAS_P(p, const_cast<uint64_t&>(all_set), all_zero);
        ++p;
      }

      size_t ptrOffset = ( (freeFirstChunk - _control) * 64) * _chunkSize.value();
      return Block(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * _chunkSize.value());
    }

    Block allocateWithRegisterOverlap(size_t numberOfBlocks) {
      // search for free area
      static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t), 
        "Current assumption that std::atomic has no overhead on integral types is not fullfilled!");

      auto p = reinterpret_cast<unsigned char*>(_control);
      const auto lastp = reinterpret_cast<unsigned char*>(_control) + _controlSize * sizeof(uint64_t);

      int freeBlocksCount(0);
      unsigned char* chunkStart = nullptr;

      // This branch works on multiple chunks at the same time and so a real lock is necessary.
      boost::unique_lock< boost::shared_mutex > guard(_mutex);

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
        size_t ptrOffset = (chunkStart - reinterpret_cast<unsigned char*>(_control)) * 8 * _chunkSize.value();
        Block result(static_cast<char*>(_buffer.ptr) + ptrOffset, numberOfBlocks * _chunkSize.value());
        setOverMultipleRegisters<SharedHelpers::NullLock, false>(blockToContext(result));
        return result;
      }

      return Block();
    }


    void deallocateForMultipleCompleteControlRegister(const BlockContext& context) {
      const auto registerToFree = context.registerIndex + context.usedChunks/ 64;
      for(auto i = context.registerIndex; i < registerToFree; i++) {
        // it is not necessary to use a unique lock is used here
        boost::shared_lock< boost::shared_mutex > guard(_mutex);
        _control[i] = static_cast<uint64_t>(-1);
      }
    }

    void deallocateWithControlRegisterOverlap(const BlockContext& context) {
      setOverMultipleRegisters<SharedHelpers::SharedLock, true>(context);
    }

  };
}

#undef CAS
#undef CAD_P