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

#include "allocator_base.hpp"

#include "internal/dynastic.hpp"
#include "internal/reallocator.hpp"
#include "internal/heap_helpers.hpp"

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/config.hpp>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace alb {
  /**
   * The Heap implements a classic heap with a pre-allocated size of
   * _numberOfChunks.value() * _chunkSize.value()
   * It has a overhead of one bit per block and linear complexity for allocation
   * and deallocation operations.
   *
   * \ingroup group_allocators
   */
  template <class Allocator, size_t NumberOfChunks, size_t ChunkSize> class heap {
    internal::dynastic<(NumberOfChunks == internal::DynasticDynamicSet ? 0 : NumberOfChunks), 0>
        _numberOfChunks;

    internal::dynastic<(ChunkSize == internal::DynasticDynamicSet ? 0 : ChunkSize), 0> _chunkSize;

    block _buffer;
    block _controlBuffer;

    // bit field where 0 means used and 1 means free block
    uint64_t *_control;
    size_t _controlSize;

    Allocator _allocator;

    void shrink()
    {
      _allocator.deallocate(_controlBuffer);
      _allocator.deallocate(_buffer);
      _control = nullptr;
    }

    heap(const heap &) = delete;
    heap &operator=(const heap &) = delete;

  public:
    static const bool supports_truncated_deallocation = true;

    using allocator = Allocator;

    heap()
    {
      init();
    }

    heap(size_t numberOfChunks, size_t chunkSize)
    {
      _numberOfChunks.value(internal::roundToAlignment(64, numberOfChunks));
      _chunkSize.value(internal::roundToAlignment(4, chunkSize));
      init();
    }

    heap(heap &&x)
    {
      *this = std::move(x);
    }

    heap &operator=(heap &&x)
    {
      if (this == &x) {
        return *this;
      }
      shrink();
      _numberOfChunks = std::move(x._numberOfChunks);
      _chunkSize = std::move(x._chunkSize);
      _buffer = std::move(x._buffer);
      _controlBuffer = std::move(x._controlBuffer);
      _control = std::move(x._control);
      _controlSize = std::move(x._controlSize);
      _allocator = std::move(x._allocator);

      x._control = nullptr;

      return *this;
    }

    size_t number_of_chunk() const
    {
      return _numberOfChunks.value();
    }

    size_t chunk_size() const
    {
      return _chunkSize.value();
    }

    ~heap()
    {
      shrink();
    }

    bool owns(const block &b) const
    {
      return b && _buffer.ptr <= b.ptr &&
             b.ptr < (static_cast<char *>(_buffer.ptr) + _buffer.length);
    }

    block allocate(size_t n)
    {
      if (n == 0) {
        return {};
      }

      // The heap cannot handle such a big request
      if (n > _chunkSize.value() * _numberOfChunks.value()) {
        return {};
      }

      size_t numberOfAlignedBytes = internal::roundToAlignment(_chunkSize.value(), n);
      size_t numberOfBlocks = numberOfAlignedBytes / _chunkSize.value();
      numberOfBlocks = std::max(size_t(1), numberOfBlocks);

      if (numberOfBlocks < 64) {
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
      else if ((numberOfBlocks % 64) == 0) {
        auto result = allocateMultipleCompleteControlRegisters(numberOfBlocks);
        if (result) {
          return result;
        }
      }
      return allocateWithRegisterOverlap(numberOfBlocks);
    }

    void deallocate(block &b)
    {
      if (!b) {
        return;
      }

      if (!owns(b)) {
        return;
      }

      const auto context = blockToContext(b);

      if (context.subIndex + context.usedChunks <= 64) {
        setWithinSingleRegister<true>(context);
      }
      else if ((context.usedChunks % 64) == 0) {
        deallocateForMultipleCompleteControlRegister(context);
      }
      else {
        deallocateWithControlRegisterOverlap(context);
      }
      b.reset();
    }

    void deallocateAll()
    {
      std::fill(_control, _control + _controlSize, (uint64_t)-1);
    }

    bool reallocate(block &b, size_t n)
    {
      if (internal::reallocator<heap>::isHandledDefault(*this, b, n)) {
        return true;
      }

      const auto numberOfBlocks = static_cast<int>(b.length / _chunkSize.value());
      const auto numberOfNewNeededBlocks =
          static_cast<int>(internal::roundToAlignment(_chunkSize.value(), n) / _chunkSize.value());

      if (numberOfBlocks == numberOfNewNeededBlocks) {
        return true;
      }
      if (b.length > n) {
        auto context = blockToContext(b);
        if (context.subIndex + context.usedChunks <= 64) {
          setWithinSingleRegister<true>(BlockContext(context.registerIndex,
                                                     context.subIndex + numberOfNewNeededBlocks,
                                                     context.usedChunks - numberOfNewNeededBlocks));
        }
        else {
          deallocateWithControlRegisterOverlap(
              BlockContext(context.registerIndex, context.subIndex + numberOfNewNeededBlocks,
                           context.usedChunks - numberOfNewNeededBlocks));
        }
        b.length = numberOfNewNeededBlocks * _chunkSize.value();
        return true;
      }
      return internal::reallocateWithCopy(*this, *this, b, n);
    }

    bool expand(block &b, size_t delta)
    {
      if (delta == 0) {
        return true;
      }

      const auto context = blockToContext(b);
      const auto numberOfAdditionalNeededBlocks = static_cast<int>(
          internal::roundToAlignment(_chunkSize.value(), delta) / _chunkSize.value());

      if (context.subIndex + context.usedChunks + numberOfAdditionalNeededBlocks <= 64) {
        if (testAndSetWithinSingleRegister<false>(
                BlockContext(context.registerIndex, context.subIndex + context.usedChunks,
                             numberOfAdditionalNeededBlocks))) {
          b.length += numberOfAdditionalNeededBlocks * _chunkSize.value();
          return true;
        }
        return false;
      }
      else {
        if (testAndSetOverMultipleRegisters<false>(
                BlockContext(context.registerIndex, context.subIndex + context.usedChunks,
                             numberOfAdditionalNeededBlocks))) {
          b.length += numberOfAdditionalNeededBlocks * _chunkSize.value();
          return true;
        }
      }
      return false;
    }

  private:
    void init()
    {
      _controlSize = _numberOfChunks.value() / 64;
      _controlBuffer = _allocator.allocate(sizeof(uint64_t) * _controlSize);
      BOOST_ASSERT((bool)_controlBuffer);

      _control = static_cast<uint64_t *>(_controlBuffer.ptr);
      _buffer = _allocator.allocate(_chunkSize.value() * _numberOfChunks.value());
      BOOST_ASSERT((bool)_buffer);

      deallocateAll();
    }

    struct BlockContext {
      BlockContext(int ri, int si, int uc)
        : registerIndex(ri)
        , subIndex(si)
        , usedChunks(uc)
      {
      }

      int registerIndex, subIndex, usedChunks;
    };

    BlockContext blockToContext(const block &b)
    {
      const auto blockIndex = static_cast<int>(
          (static_cast<char *>(b.ptr) - static_cast<char *>(_buffer.ptr)) / _chunkSize.value());

      return {blockIndex / 64, blockIndex % 64, static_cast<int>(b.length / _chunkSize.value())};
    }

    template <bool Used> bool testAndSetWithinSingleRegister(const BlockContext &context)
    {
      BOOST_ASSERT(context.subIndex + context.usedChunks <= 64);

      uint64_t mask = (context.usedChunks == 64)
                          ? (uint64_t)-1
                          : (((1uLL << context.usedChunks) - 1) << context.subIndex);

      uint64_t currentRegister, newRegister;
      currentRegister = _control[context.registerIndex];
      if ((currentRegister & mask) != mask) {
        return false;
      }
      newRegister = Helpers::setUsed<Used>(currentRegister, mask);
      _control[context.registerIndex] = newRegister;
      return true;
    }

    template <bool Used> void setOverMultipleRegisters(const BlockContext &context)
    {
      size_t chunksToTest = context.usedChunks;
      size_t subIndexStart = context.subIndex;
      size_t registerIndex = context.registerIndex;
      do {
        size_t mask;
        if (subIndexStart > 0)
          mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
        else
          mask = (chunksToTest >= 64) ? (uint64_t)-1 : ((1uLL << chunksToTest) - 1);

        BOOST_ASSERT(registerIndex < _controlSize);

        uint64_t currentRegister, newRegister;

        currentRegister = _control[registerIndex];
        newRegister = Helpers::setUsed<Used>(currentRegister, mask);
        _control[registerIndex] = newRegister;

        if (subIndexStart + chunksToTest > 64) {
          chunksToTest = subIndexStart + chunksToTest - 64;
          subIndexStart = 0;
        }
        else {
          chunksToTest = 0;
        }
        registerIndex++;
      } while (chunksToTest > 0);
    }

    template <bool Used> bool testAndSetOverMultipleRegisters(const BlockContext &context)
    {
      // This branch works on multiple chunks at the same time and so a real lock
      // is necessary.
      size_t chunksToTest = context.usedChunks;
      size_t subIndexStart = context.subIndex;
      size_t registerIndex = context.registerIndex;

      do {
        uint64_t mask;
        if (subIndexStart > 0)
          mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
        else
          mask = (chunksToTest >= 64) ? (uint64_t)-1 : ((1uLL << chunksToTest) - 1);

        auto currentRegister = _control[registerIndex];

        if ((currentRegister & mask) != mask) {
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
      } while (chunksToTest > 0);

      setOverMultipleRegisters<Used>(context);

      return true;
    }

    template <bool Used> void setWithinSingleRegister(const BlockContext &context)
    {
      BOOST_ASSERT(context.subIndex + context.usedChunks <= 64);

      uint64_t mask = (context.usedChunks == 64)
                          ? (uint64_t)-1
                          : (((1uLL << context.usedChunks) - 1) << context.subIndex);

      uint64_t currentRegister, newRegister;
      currentRegister = _control[context.registerIndex];
      newRegister = Helpers::setUsed<Used>(currentRegister, mask);
      _control[context.registerIndex] = newRegister;
    }

    block allocateWithinASingleControlRegister(size_t numberOfBlocks)
    {
      // first we have to look for at least one free block
      int controlIndex = 0;
      while (controlIndex < _controlSize) {
        auto currentControlRegister = _control[controlIndex];

        // == 0 means that all blocks are in use and no need to search further
        if (currentControlRegister != 0) {
          uint64_t mask = (numberOfBlocks == 64) ? (uint64_t)-1 : ((1uLL << numberOfBlocks) - 1);

          int i = 0;
          // Search for numberOfBlock bits that are set to one
          while (i <= 64 - numberOfBlocks) {
            if ((currentControlRegister & mask) == mask) {
              auto newControlRegister = Helpers::setUsed<false>(currentControlRegister, mask);

              _control[controlIndex] = newControlRegister;

              size_t ptrOffset = (controlIndex * 64 + i) * _chunkSize.value();

              return {static_cast<char *>(_buffer.ptr) + ptrOffset,
                      numberOfBlocks * _chunkSize.value()};
            }
            i++;
            mask <<= 1;
          };
        }
        controlIndex++;
      }
      return {};
    }

    block allocateWithinCompleteControlRegister(size_t numberOfBlocks)
    {
      // first we have to look for at least full free block
      auto freeChunk = std::find_if(_control, _control + _controlSize,
                                    [this](const uint64_t &v) { return v == (uint64_t)-1; });

      if (freeChunk == _control + _controlSize) {
        return {};
      }

      *freeChunk = 0;
      size_t ptrOffset = ((freeChunk - _control) * 64) * _chunkSize.value();

      return {static_cast<char *>(_buffer.ptr) + ptrOffset, numberOfBlocks * _chunkSize.value()};
    }

    block allocateMultipleCompleteControlRegisters(size_t numberOfBlocks)
    {
      const auto neededChunks = static_cast<int>(numberOfBlocks / 64);
      auto freeFirstChunk =
          std::search_n(_control, _control + _controlSize, neededChunks, (uint64_t)-1,
                        [](uint64_t const &v, uint64_t const &p) { return v == p; });

      if (freeFirstChunk == _control + _controlSize) {
        return {};
      }
      auto p = freeFirstChunk;
      while (p < freeFirstChunk + neededChunks) {
        *p = 0;
        ++p;
      }

      size_t ptrOffset = ((freeFirstChunk - _control) * 64) * _chunkSize.value();
      return block(static_cast<char *>(_buffer.ptr) + ptrOffset,
                   numberOfBlocks * _chunkSize.value());
    }

    block allocateWithRegisterOverlap(size_t numberOfBlocks)
    {
      // search for free area
      auto p = reinterpret_cast<unsigned char *>(_control);
      const auto lastp =
          reinterpret_cast<unsigned char *>(_control) + _controlSize * sizeof(uint64_t);

      int freeBlocksCount(0);
      unsigned char *chunkStart = nullptr;

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
        size_t ptrOffset =
            (chunkStart - reinterpret_cast<unsigned char *>(_control)) * 8 * _chunkSize.value();

        block result{static_cast<char *>(_buffer.ptr) + ptrOffset,
                     numberOfBlocks * _chunkSize.value()};

        setOverMultipleRegisters<false>(blockToContext(result));
        return result;
      }

      return {};
    }

    void deallocateForMultipleCompleteControlRegister(const BlockContext &context)
    {
      const auto registerToFree = context.registerIndex + context.usedChunks / 64;
      for (auto i = context.registerIndex; i < registerToFree; i++) {
        _control[i] = (uint64_t)-1;
      }
    }

    void deallocateWithControlRegisterOverlap(const BlockContext &context)
    {
      setOverMultipleRegisters<true>(context);
    }
  };
}
