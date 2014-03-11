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
#include "internal/shared_helpers.hpp"
#include "internal/dynastic.hpp"
#include "internal/reallocator.hpp"
#include "internal/heap_helpers.hpp"

#include <atomic>
#include <algorithm>
#include <boost/thread.hpp>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
#include <boost/noncopyable.hpp>
#endif

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

#define CAS(ATOMIC, EXPECT, VALUE) ATOMIC.compare_exchange_strong(EXPECT, VALUE)

#define CAS_P(ATOMIC, EXPECT, VALUE)                                           \
  ATOMIC->compare_exchange_strong(EXPECT, VALUE)

namespace alb {

  /**
 * The SharedHeap implements a classic heap with a pre-allocated size of
 * _numberOfChunks.value() * _chunkSize.value()
 * It has a overhead of one bit per block and linear complexity for allocation
 * and deallocation operations.
 * It is thread safe, except the moment of instantiation.
 * As far as possible only a shared lock + an atomic operation is used during
 * the memory operations
 *
 * \ingroup group_allocators group_shared
 */
template <class Allocator, size_t NumberOfChunks, size_t ChunkSize>
class shared_heap 
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
  : boost::noncopyable
#endif
{
  const uint64_t all_set;
  const uint64_t all_zero;

  internal::dynastic<
    (NumberOfChunks == internal::DynasticDynamicSet ? 0 : NumberOfChunks), 0>
    _numberOfChunks;

  internal::dynastic<
    (ChunkSize == internal::DynasticDynamicSet ? 0 : ChunkSize), 0>
    _chunkSize;

  block _buffer;
  block _controlBuffer;

  // bit field where 0 means used and 1 means free block
  std::atomic<uint64_t> *_control;
  size_t _controlSize;

  boost::shared_mutex _mutex;
  Allocator _allocator;

  void shrink() {
    _allocator.deallocate(_controlBuffer);
    _allocator.deallocate(_buffer);
    _control = nullptr;
  }

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
  shared_heap(const shared_heap&) = delete;
  const shared_heap& operator=(const shared_heap&) = delete;
#endif

public:
  BOOST_STATIC_CONSTANT(bool, supports_truncated_deallocation = true);

  typedef Allocator allocator;

  shared_heap() 
    : all_set(static_cast<uint64_t>(-1))
    , all_zero(0) {
    init();
  }

  shared_heap(size_t numberOfChunks, size_t chunkSize)
    : all_set(static_cast<uint64_t>(-1))
    , all_zero(0) {
    _numberOfChunks.value(internal::roundToAlignment(64, numberOfChunks));
    _chunkSize.value(internal::roundToAlignment(4, chunkSize));
    init();
  }

  shared_heap(shared_heap&& x) {
    *this = std::move(x);
  }

  shared_heap& operator=(shared_heap&& x) {
    if (this == &x) {
      return *this;
    }
    boost::unique_lock<boost::shared_mutex> guardThis(_mutex);
    shrink();
    boost::unique_lock<boost::shared_mutex> guardX(x._mutex);
    _numberOfChunks = std::move(x._numberOfChunks);
    _chunkSize      = std::move(x._chunkSize);
    _buffer         = std::move(x._buffer);
    _controlBuffer  = std::move(x._controlBuffer);
    _control        = std::move(x._control);
    _controlSize    = std::move(x._controlSize);
    _allocator      = std::move(x._allocator);
    
    x._control = nullptr;
    
    return *this;
  }

  size_t number_of_chunk() const { return _numberOfChunks.value(); }

  size_t chunk_size() const { return _chunkSize.value(); }

  ~shared_heap() {
    boost::unique_lock<boost::shared_mutex> guard(_mutex);
    shrink();
  }

  bool owns(const block &b) const {
    return b && _buffer.ptr <= b.ptr &&
           b.ptr < (static_cast<char *>(_buffer.ptr) + _buffer.length);
  }

  block allocate(size_t n) {
    if (n == 0) {
      return block();
    }

    // The heap cannot handle such a big request
    if (n > _chunkSize.value() * _numberOfChunks.value()) {
      return block();
    }

    size_t numberOfAlignedBytes =
        internal::roundToAlignment(_chunkSize.value(), n);
    size_t numberOfBlocks = numberOfAlignedBytes / _chunkSize.value();
    numberOfBlocks = std::max(static_cast<size_t>(1), numberOfBlocks);

    if (numberOfBlocks < 64) {
      auto result = allocateWithinASingleControlRegister(numberOfBlocks);
      if (result) {
        return result;
      }
    } else if (numberOfBlocks == 64) {
      auto result = allocateWithinCompleteControlRegister(numberOfBlocks);
      if (result) {
        return result;
      }
    } else if ((numberOfBlocks % 64) == 0) {
      auto result = allocateMultipleCompleteControlRegisters(numberOfBlocks);
      if (result) {
        return result;
      }
    }
    return allocateWithRegisterOverlap(numberOfBlocks);
  }

  void deallocate(block &b) {
    if (!b) {
      return;
    }
    BOOST_ASSERT_MSG(owns(b),
                     "It is not wise to let me deallocate a foreign Block!");
    if (!owns(b)) {
      return;
    }

    const auto context = blockToContext(b);

    // printf("Used Block %d in thread %d\n", blockIndex,
    // std::this_thread::get_id());
    if (context.subIndex + context.usedChunks <= 64) {
      setWithinSingleRegister<shared_helpers::SharedLock, true>(context);
    } else if ((context.usedChunks % 64) == 0) {
      deallocateForMultipleCompleteControlRegister(context);
    } else {
      deallocateWithControlRegisterOverlap(context);
    }
    b.reset();
  }

  void deallocateAll() {
    boost::unique_lock<boost::shared_mutex> guard(_mutex);

    std::fill(_control, _control + _controlSize, static_cast<uint64_t>(-1));
  }

  bool reallocate(block &b, size_t n) {
    if (internal::reallocator<shared_heap>::isHandledDefault(*this, b, n)) {
      return true;
    }

    const int numberOfBlocks = static_cast<int>(b.length / _chunkSize.value());
    const int numberOfNewNeededBlocks = static_cast<int>(
        internal::roundToAlignment(_chunkSize.value(), n) / _chunkSize.value());

    if (numberOfBlocks == numberOfNewNeededBlocks) {
      return true;
    }
    if (b.length > n) {
      auto context = blockToContext(b);
      if (context.subIndex + context.usedChunks <= 64) {
        setWithinSingleRegister<shared_helpers::SharedLock, true>(BlockContext(
            context.registerIndex, context.subIndex + numberOfNewNeededBlocks,
            context.usedChunks - numberOfNewNeededBlocks));
      } else {
        deallocateWithControlRegisterOverlap(BlockContext(
            context.registerIndex, context.subIndex + numberOfNewNeededBlocks,
            context.usedChunks - numberOfNewNeededBlocks));
      }
      b.length = numberOfNewNeededBlocks * _chunkSize.value();
      return true;
    }
    return internal::reallocateWithCopy(*this, *this, b, n);
  }

  bool expand(block &b, size_t delta) {
    if (delta == 0) {
      return true;
    }

    const auto context = blockToContext(b);
    const int numberOfAdditionalNeededBlocks =
        static_cast<int>(internal::roundToAlignment(_chunkSize.value(), delta) /
                         _chunkSize.value());

    if (context.subIndex + context.usedChunks +
            numberOfAdditionalNeededBlocks <=
        64) {
      if (testAndSetWithinSingleRegister<false>(BlockContext(
              context.registerIndex, context.subIndex + context.usedChunks,
              numberOfAdditionalNeededBlocks))) {
        b.length += numberOfAdditionalNeededBlocks * _chunkSize.value();
        return true;
      } else {
        return false;
      }
    } else {
      if (testAndSetOverMultipleRegisters<false>(BlockContext(
              context.registerIndex, context.subIndex + context.usedChunks,
              numberOfAdditionalNeededBlocks))) {
        b.length += numberOfAdditionalNeededBlocks * _chunkSize.value();
        return true;
      }
    }
    return false;
  }

private:
  void init() {
    _controlSize = _numberOfChunks.value() / 64;
    _controlBuffer =
        _allocator.allocate(sizeof(std::atomic<uint64_t>) * _controlSize);
    BOOST_ASSERT((bool)_controlBuffer);

    _control = static_cast<std::atomic<uint64_t> *>(_controlBuffer.ptr);
    new (_control) std::atomic<uint64_t>[_controlSize]();

    _buffer = _allocator.allocate(_chunkSize.value() * _numberOfChunks.value());
    BOOST_ASSERT((bool)_buffer);

    deallocateAll();
  }

  struct BlockContext {
    BlockContext(int ri, int si, int uc)
        : registerIndex(ri), subIndex(si), usedChunks(uc) {}
    int registerIndex, subIndex, usedChunks;
  };

  BlockContext blockToContext(const block &b) {
    const int blockIndex = static_cast<int>(
        (static_cast<char *>(b.ptr) - static_cast<char *>(_buffer.ptr)) /
        _chunkSize.value());

    return BlockContext(blockIndex / 64, blockIndex % 64,
                        static_cast<int>(b.length / _chunkSize.value()));
  }

  template <bool Used>
  bool testAndSetWithinSingleRegister(const BlockContext &context) {
    BOOST_ASSERT(context.subIndex + context.usedChunks <= 64);

    uint64_t mask =
        (context.usedChunks == 64)
            ? all_set
            : (((1uLL << context.usedChunks) - 1) << context.subIndex);

    uint64_t currentRegister, newRegister;
    do {
      currentRegister = _control[context.registerIndex].load();
      if ((currentRegister & mask) != mask) {
        return false;
      }
      newRegister = Helpers::setUsed<Used>(currentRegister, mask);
      boost::shared_lock<boost::shared_mutex> guard(_mutex);
    } while (
        !CAS(_control[context.registerIndex], currentRegister, newRegister));
    return true;
  }

  template <class LockPolicy, bool Used>
  void setOverMultipleRegisters(const BlockContext &context) {
    size_t chunksToTest = context.usedChunks;
    size_t subIndexStart = context.subIndex;
    size_t registerIndex = context.registerIndex;
    do {
      size_t mask;
      if (subIndexStart > 0)
        mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
      else
        mask = (chunksToTest >= 64) ? all_set : ((1uLL << chunksToTest) - 1);

      BOOST_ASSERT(registerIndex < _controlSize);

      uint64_t currentRegister, newRegister;
      do {
        currentRegister = _control[registerIndex].load();
        newRegister = Helpers::setUsed<Used>(currentRegister, mask);
        LockPolicy guard(_mutex);
      } while (!CAS(_control[registerIndex], currentRegister, newRegister));

      if (subIndexStart + chunksToTest > 64) {
        chunksToTest = subIndexStart + chunksToTest - 64;
        subIndexStart = 0;
      } else {
        chunksToTest = 0;
      }
      registerIndex++;
    } while (chunksToTest > 0);
  }

  template <bool Used>
  bool testAndSetOverMultipleRegisters(const BlockContext &context) {
    static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t),
                  "Current assumption that std::atomic has no overhead on "
                  "integral types is not fullfilled!");

    // This branch works on multiple chunks at the same time and so a real lock
    // is necessary.
    size_t chunksToTest = context.usedChunks;
    size_t subIndexStart = context.subIndex;
    size_t registerIndex = context.registerIndex;

    boost::unique_lock<boost::shared_mutex> guard(_mutex);
    do {
      uint64_t mask;
      if (subIndexStart > 0)
        mask = ((1uLL << (64 - subIndexStart)) - 1) << subIndexStart;
      else
        mask = (chunksToTest >= 64) ? all_set : ((1uLL << chunksToTest) - 1);

      auto currentRegister = _control[registerIndex].load();

      if ((currentRegister & mask) != mask) {
        return false;
      }

      if (subIndexStart + chunksToTest > 64) {
        chunksToTest = subIndexStart + chunksToTest - 64;
        subIndexStart = 0;
      } else {
        chunksToTest = 0;
      }
      registerIndex++;
      if (registerIndex > _controlSize) {
        return false;
      }
    } while (chunksToTest > 0);

    setOverMultipleRegisters<shared_helpers::NullLock, Used>(context);

    return true;
  }

  template <class LockPolicy, bool Used>
  void setWithinSingleRegister(const BlockContext &context) {
    BOOST_ASSERT(context.subIndex + context.usedChunks <= 64);

    uint64_t mask =
        (context.usedChunks == 64)
            ? all_set
            : (((1uLL << context.usedChunks) - 1) << context.subIndex);

    uint64_t currentRegister, newRegister;
    do {
      currentRegister = _control[context.registerIndex].load();
      newRegister = Helpers::setUsed<Used>(currentRegister, mask);
      LockPolicy guard(_mutex);
    } while (
        !CAS(_control[context.registerIndex], currentRegister, newRegister));
  }

  block allocateWithinASingleControlRegister(size_t numberOfBlocks) {
    // we must assume that we may find a free location, but that it is later
    // already used during the set operation
    do {
      // first we have to look for at least one free block
      int controlIndex = 0;
      while (controlIndex < _controlSize) {
        auto currentControlRegister = _control[controlIndex].load();

        // == 0 means that all blocks are in use and no need to search further
        if (currentControlRegister != 0) {
          uint64_t mask =
              (numberOfBlocks == 64) ? all_set : ((1uLL << numberOfBlocks) - 1);

          int i = 0;
          // Search for numberOfBlock bits that are set to one
          while (i <= 64 - numberOfBlocks) {
            if ((currentControlRegister & mask) == mask) {
              auto newControlRegister =
                  Helpers::setUsed<false>(currentControlRegister, mask);

              boost::shared_lock<boost::shared_mutex> guard(_mutex);

              if (CAS(_control[controlIndex], currentControlRegister,
                      newControlRegister)) {
                size_t ptrOffset = (controlIndex * 64 + i) * _chunkSize.value();

                return block(static_cast<char *>(_buffer.ptr) + ptrOffset,
                             numberOfBlocks * _chunkSize.value());
              }
            }
            i++;
            mask <<= 1;
          };
        }
        controlIndex++;
      }
      if (controlIndex == _controlSize) {
        return block();
      }
    } while (true);
  }

  block allocateWithinCompleteControlRegister(size_t numberOfBlocks) {
    // we must assume that we may find a free location, but that it is later
    // already used during the CAS set operation
    do {
      // first we have to look for at least full free block
      auto freeChunk = std::find_if(_control, _control + _controlSize,
                                    [this](const std::atomic<uint64_t> &v) {
        return v.load() == all_set;
      });

      if (freeChunk == _control + _controlSize) {
        return block();
      }

      boost::shared_lock<boost::shared_mutex> guard(_mutex);

      if (CAS_P(freeChunk, const_cast<uint64_t &>(all_set), all_zero)) {
        size_t ptrOffset = ((freeChunk - _control) * 64) * _chunkSize.value();

        return block(static_cast<char *>(_buffer.ptr) + ptrOffset,
                     numberOfBlocks * _chunkSize.value());
      }
    } while (true);
  }

  block allocateMultipleCompleteControlRegisters(size_t numberOfBlocks) {
    // This branch works on multiple chunks at the same time and so a real
    // lock is necessary.
    boost::unique_lock<boost::shared_mutex> guard(_mutex);

    const int neededChunks = static_cast<int>(numberOfBlocks / 64);
    auto freeFirstChunk =
        std::search_n(_control, _control + _controlSize, neededChunks, all_set,
                      [](const std::atomic<uint64_t> &v,
                         const uint64_t &p) { return v.load() == p; });

    if (freeFirstChunk == _control + _controlSize) {
      return block();
    }
    auto p = freeFirstChunk;
    while (p < freeFirstChunk + neededChunks) {
      CAS_P(p, const_cast<uint64_t &>(all_set), all_zero);
      ++p;
    }

    size_t ptrOffset = ((freeFirstChunk - _control) * 64) * _chunkSize.value();
    return block(static_cast<char *>(_buffer.ptr) + ptrOffset,
                 numberOfBlocks * _chunkSize.value());
  }

  block allocateWithRegisterOverlap(size_t numberOfBlocks) {
    // search for free area
    static_assert(sizeof(std::atomic<uint64_t>) == sizeof(uint64_t),
                  "Current assumption that std::atomic has no overhead on "
                  "integral types is not fullfilled!");

    auto p = reinterpret_cast<unsigned char *>(_control);
    const auto lastp = reinterpret_cast<unsigned char *>(_control) +
                       _controlSize * sizeof(uint64_t);

    int freeBlocksCount(0);
    unsigned char *chunkStart = nullptr;

    // This branch works on multiple chunks at the same time and so a real
    // lock is necessary.
    boost::unique_lock<boost::shared_mutex> guard(_mutex);

    while (p < lastp) {
      if (*p == 0xff) { // free
        if (!chunkStart) {
          chunkStart = p;
        }

        freeBlocksCount += 8;
        if (freeBlocksCount >= numberOfBlocks) {
          break;
        }
      } else {
        freeBlocksCount = 0;
        chunkStart = nullptr;
      }
      p++;
    };

    if (p != lastp && freeBlocksCount >= numberOfBlocks) {
      size_t ptrOffset =
          (chunkStart - reinterpret_cast<unsigned char *>(_control)) * 8 *
          _chunkSize.value();

      block result(static_cast<char *>(_buffer.ptr) + ptrOffset,
                   numberOfBlocks * _chunkSize.value());

      setOverMultipleRegisters<shared_helpers::NullLock, false>(
          blockToContext(result));
      return result;
    }

    return block();
  }

  void
  deallocateForMultipleCompleteControlRegister(const BlockContext &context) {
    const auto registerToFree = context.registerIndex + context.usedChunks / 64;
    for (auto i = context.registerIndex; i < registerToFree; i++) {
      // it is not necessary to use a unique lock is used here
      boost::shared_lock<boost::shared_mutex> guard(_mutex);
      _control[i] = static_cast<uint64_t>(-1);
    }
  }

  void deallocateWithControlRegisterOverlap(const BlockContext &context) {
    setOverMultipleRegisters<shared_helpers::SharedLock, true>(context);
  }
};
}

#undef CAS
#undef CAS_P