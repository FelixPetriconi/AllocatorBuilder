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
#include <cassert>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

namespace alb {
  inline namespace v_100 {
    /**
     * The Heap implements a classic heap with a pre-allocated size of
     * numberOfChunks_.value() * chunk_size_.value()
     * It has a overhead of one bit per block and linear complexity for allocation
     * and deallocation operations.
     *
     * \ingroup group_allocators
     */
    template <class Allocator, size_t NumberOfChunks, size_t ChunkSize> 
    class heap 
    {
      internal::dynastic<(NumberOfChunks == internal::DynasticDynamicSet ? 0 : NumberOfChunks), 0>
        numberOfChunks_;

      internal::dynastic<(ChunkSize == internal::DynasticDynamicSet ? 0 : ChunkSize), 0> chunk_size_;

      block buffer_;
      block controlBuffer_;

      // bit field where 0 means used and 1 means free block
      const uint64_t all_set = std::numeric_limits<uint64_t>::max();
      const uint64_t all_zero = uint64_t(0);

      uint64_t *control_;
      size_t controlSize_;

      Allocator allocator_;

      void shrink() noexcept {
        allocator_.deallocate(controlBuffer_);
        allocator_.deallocate(buffer_);
        control_ = nullptr;
      }

      heap(const heap &) = delete;
      heap &operator=(const heap &) = delete;

    public:
      using allocator = Allocator;

      static constexpr bool supports_truncated_deallocation = true;
      static constexpr unsigned alignment = Allocator::alignment;

      heap() noexcept {
        init();
      }

      heap(size_t numberOfChunks, size_t chunkSize) noexcept {
        numberOfChunks_.value(internal::round_to_alignment(64, numberOfChunks));
        chunk_size_.value(internal::round_to_alignment(4, chunkSize));
        init();
      }

      heap(heap &&x) noexcept {
        *this = std::move(x);
      }

      heap &operator=(heap &&x) noexcept {
        if (this == &x) {
          return *this;
        }
        shrink();
        numberOfChunks_ = std::move(x.numberOfChunks_);
        chunk_size_     = std::move(x.chunk_size_);
        buffer_         = std::move(x.buffer_);
        controlBuffer_  = std::move(x.controlBuffer_);
        control_        = std::move(x.control_);
        controlSize_    = std::move(x.controlSize_);
        allocator_      = std::move(x.allocator_);

        x.control_      = nullptr;

        return *this;
      }

      ~heap() {
        shrink();
      }

      size_t number_of_chunk() const noexcept {
        return numberOfChunks_.value();
      }

      size_t chunk_size() const noexcept {
        return chunk_size_.value();
      }

      bool owns(const block &b) const noexcept {
        return b && buffer_.ptr <= b.ptr &&
          b.ptr < (static_cast<char *>(buffer_.ptr) + buffer_.length);
      }

      block allocate(size_t n) noexcept {
        block result;
        if (n == 0) {
          return result;
        }

        // The heap cannot handle such a big request
        if (n > chunk_size_.value() * numberOfChunks_.value()) {
          return result;
        }

        size_t numberOfAlignedBytes = internal::round_to_alignment(chunk_size_.value(), n);
        size_t numberOfBlocks = numberOfAlignedBytes / chunk_size_.value();
        numberOfBlocks = std::max(size_t(1), numberOfBlocks);

        if (numberOfBlocks < 64) {
          result = allocate_within_single_control_register(numberOfBlocks);
          if (result) {
            return result;
          }
        }
        else if (numberOfBlocks == 64) {
          result = allocate_within_complete_control_register(numberOfBlocks);
          if (result) {
            return result;
          }
        }
        else if ((numberOfBlocks % 64) == 0) {
          result = allocate_multiple_complete_control_registers(numberOfBlocks);
          if (result) {
            return result;
          }
        }

        result = allocate_with_register_overlap(numberOfBlocks);
        return result;
      }

      void deallocate(block &b) noexcept {
        if (!b) {
          return;
        }

        if (!owns(b)) {
          return;
        }

        const auto context = block_to_context(b);

        if (context.subIndex + context.usedChunks <= 64) {
          set_within_single_register<true>(context);
        }
        else if ((context.usedChunks % 64) == 0) {
          deallocate_for_multiple_complete_control_register(context);
        }
        else {
          deallocate_with_control_register_overlap(context);
        }
        b.reset();
      }

      void deallocate_all() noexcept {
        std::fill(control_, control_ + controlSize_, all_set);
      }

      bool reallocate(block &b, size_t n) noexcept {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
          return true;
        }

        const auto numberOfBlocks = static_cast<int>(b.length / chunk_size_.value());
        const auto numberOfNewNeededBlocks =
          static_cast<int>(internal::round_to_alignment(chunk_size_.value(), n) / chunk_size_.value());

        if (numberOfBlocks == numberOfNewNeededBlocks) {
          return true;
        }
        if (b.length > n) {
          auto context = block_to_context(b);
          if (context.subIndex + context.usedChunks <= 64) {
            set_within_single_register<true>(BlockContext{ context.registerIndex,
                                                       context.subIndex + numberOfNewNeededBlocks,
                                                       context.usedChunks - numberOfNewNeededBlocks });
          }
          else {
            deallocate_with_control_register_overlap(
              BlockContext{ context.registerIndex, context.subIndex + numberOfNewNeededBlocks,
                           context.usedChunks - numberOfNewNeededBlocks });
          }
          b.length = numberOfNewNeededBlocks * chunk_size_.value();
          return true;
        }
        return internal::reallocate_with_copy(*this, *this, b, n);
      }

      bool expand(block &b, size_t delta) noexcept {
        if (delta == 0) {
          return true;
        }

        const auto context = block_to_context(b);
        const auto numberOfAdditionalNeededBlocks = static_cast<int>(
          internal::round_to_alignment(chunk_size_.value(), delta) / chunk_size_.value());

        if (context.subIndex + context.usedChunks + numberOfAdditionalNeededBlocks <= 64) {
          if (test_and_set_within_single_register<false>(
            BlockContext{ context.registerIndex, context.subIndex + context.usedChunks,
                         numberOfAdditionalNeededBlocks })) {
            b.length += numberOfAdditionalNeededBlocks * chunk_size_.value();
            return true;
          }
          return false;
        }
        else {
          if (test_and_set_over_multiple_registers<false>(
            BlockContext{ context.registerIndex, context.subIndex + context.usedChunks,
                         numberOfAdditionalNeededBlocks })) {
            b.length += numberOfAdditionalNeededBlocks * chunk_size_.value();
            return true;
          }
        }
        return false;
      }

    private:
      void init() noexcept {
        assert(chunk_size_.value() > alignment);
        assert(chunk_size_.value() % alignment == 0);

        controlSize_ = numberOfChunks_.value() / 64;
        controlBuffer_ = allocator_.allocate(sizeof(uint64_t) * controlSize_);
        assert((bool)controlBuffer_);

        control_ = static_cast<uint64_t *>(controlBuffer_.ptr);
        buffer_ = allocator_.allocate(chunk_size_.value() * numberOfChunks_.value());
        assert((bool)buffer_);

        deallocate_all();
      }

      struct BlockContext 
      {
        int registerIndex;
        int subIndex;
        int usedChunks;
      };

      BlockContext block_to_context(const block &b)  noexcept {
        const auto blockIndex = static_cast<int>(
          (static_cast<char *>(b.ptr) - static_cast<char *>(buffer_.ptr)) / chunk_size_.value());

        return{ blockIndex / 64, blockIndex % 64, static_cast<int>(b.length / chunk_size_.value()) };
      }

      template <bool Used> 
      bool test_and_set_within_single_register(const BlockContext &context)  noexcept {
        assert(context.subIndex + context.usedChunks <= 64);

        uint64_t mask = (context.usedChunks == 64)
          ? all_set
          : (((uint64_t(1) << context.usedChunks) - 1) << context.subIndex);

        uint64_t currentRegister, newRegister;
        currentRegister = control_[context.registerIndex];
        if ((currentRegister & mask) != mask) {
          return false;
        }
        newRegister = helpers::set_used<Used>(currentRegister, mask);
        control_[context.registerIndex] = newRegister;
        return true;
      }

      template <bool Used> 
      void set_over_multiple_registers(const BlockContext &context)  noexcept {
        size_t chunksToTest = context.usedChunks;
        size_t subIndexStart = context.subIndex;
        size_t registerIndex = context.registerIndex;
        do {
          size_t mask;
          if (subIndexStart > 0)
            mask = ((uint64_t(1) << (64 - subIndexStart)) - 1) << subIndexStart;
          else
            mask = (chunksToTest >= 64) ? all_set : ((uint64_t(1) << chunksToTest) - 1);

          assert(registerIndex < controlSize_);

          uint64_t currentRegister, newRegister;

          currentRegister = control_[registerIndex];
          newRegister = helpers::set_used<Used>(currentRegister, mask);
          control_[registerIndex] = newRegister;

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

      template <bool Used> 
      bool test_and_set_over_multiple_registers(const BlockContext &context)  noexcept {
        // This branch works on multiple chunks at the same time and so a real lock
        // is necessary.
        size_t chunksToTest = context.usedChunks;
        size_t subIndexStart = context.subIndex;
        size_t registerIndex = context.registerIndex;

        do {
          uint64_t mask;
          if (subIndexStart > 0)
            mask = ((uint64_t(1) << (64 - subIndexStart)) - 1) << subIndexStart;
          else
            mask = (chunksToTest >= 64) ? all_set : ((uint64_t(1) << chunksToTest) - 1);

          auto currentRegister = control_[registerIndex];

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
          if (registerIndex > controlSize_) {
            return false;
          }
        } while (chunksToTest > 0);

        set_over_multiple_registers<Used>(context);

        return true;
      }

      template <bool Used> 
      void set_within_single_register(const BlockContext &context) noexcept {
        assert(context.subIndex + context.usedChunks <= 64);

        uint64_t mask = (context.usedChunks == 64)
          ? all_set
          : (((uint64_t(1) << context.usedChunks) - 1) << context.subIndex);

        uint64_t currentRegister, newRegister;
        currentRegister = control_[context.registerIndex];
        newRegister = helpers::set_used<Used>(currentRegister, mask);
        control_[context.registerIndex] = newRegister;
      }

      block allocate_within_single_control_register(size_t numberOfBlocks) noexcept {
        block result;

        // first we have to look for at least one free block
        size_t controlIndex = 0;
        while (controlIndex < controlSize_) {
          auto currentControlRegister = control_[controlIndex];

          // == 0 means that all blocks are in use and no need to search further
          if (currentControlRegister != 0) {
            uint64_t mask = (numberOfBlocks == 64) ? 
              all_set : ((uint64_t(1) << numberOfBlocks) - 1);

            size_t i = 0;
            // Search for numberOfBlock bits that are set to one
            while (i <= 64 - numberOfBlocks) {
              if ((currentControlRegister & mask) == mask) {
                auto newControlRegister = helpers::set_used<false>(currentControlRegister, mask);

                control_[controlIndex] = newControlRegister;

                size_t ptrOffset = (controlIndex * 64 + i) * chunk_size_.value();

                result.ptr = static_cast<char *>(buffer_.ptr) + ptrOffset;
                result.length = numberOfBlocks * chunk_size_.value();
                
                return result;
              }
              i++;
              mask <<= 1;
            };
          }
          controlIndex++;
        }

        return result;
      }

      block allocate_within_complete_control_register(size_t numberOfBlocks) noexcept {
        block result;

        // first we have to look for at least full free block
        auto freeChunk = std::find_if(control_, control_ + controlSize_,
          [this](const uint64_t &v) { return v == all_set; });

        if (freeChunk == control_ + controlSize_) {
          return result;
        }

        *freeChunk = 0;
        size_t ptrOffset = ((freeChunk - control_) * 64) * chunk_size_.value();

        result.ptr = static_cast<char *>(buffer_.ptr) + ptrOffset;
        result.length = numberOfBlocks * chunk_size_.value();

        return result;
      }

      block allocate_multiple_complete_control_registers(size_t numberOfBlocks) noexcept {
        block result;

        const auto neededChunks = static_cast<int>(numberOfBlocks / 64);
        auto freeFirstChunk =
          std::search_n(control_, control_ + controlSize_, neededChunks, all_set,
            [](uint64_t const &v, uint64_t const &p) { return v == p; });

        if (freeFirstChunk == control_ + controlSize_) {
          return result;
        }
        auto p = freeFirstChunk;
        while (p < freeFirstChunk + neededChunks) {
          *p = 0;
          ++p;
        }

        size_t ptrOffset = ((freeFirstChunk - control_) * 64) * chunk_size_.value();

        result.ptr = static_cast<char *>(buffer_.ptr) + ptrOffset;
        result.length = numberOfBlocks * chunk_size_.value();

        return result;
      }

      block allocate_with_register_overlap(size_t numberOfBlocks) noexcept {
        block result;

        // search for free area
        auto p = reinterpret_cast<unsigned char *>(control_);
        const auto lastp =
          reinterpret_cast<unsigned char *>(control_) + controlSize_ * sizeof(uint64_t);

        size_t freeBlocksCount(0);
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
            (chunkStart - reinterpret_cast<unsigned char *>(control_)) * 8 * chunk_size_.value();

          result.ptr = static_cast<char *>(buffer_.ptr) + ptrOffset;
          result.length = numberOfBlocks * chunk_size_.value();

          set_over_multiple_registers<false>(block_to_context(result));
          return result;
        }

        return result;
      }

      void deallocate_for_multiple_complete_control_register(const BlockContext &context) noexcept
      {
        const auto registerToFree = context.registerIndex + context.usedChunks / 64;
        for (auto i = context.registerIndex; i < registerToFree; ++i) {
          control_[i] = all_set;
        }
      }

      void deallocate_with_control_register_overlap(const BlockContext &context) noexcept
      {
        set_over_multiple_registers<true>(context);
      }
    };
  }
  using namespace v_100;
}
