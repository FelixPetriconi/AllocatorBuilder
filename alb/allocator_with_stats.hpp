///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors:
//          http://erdani.com, Andrei Alexandrescu
//          Implementation in D from
//          https://github.com/andralex/phobos/blob/allocator/std/allocator.d
//          http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#pragma once

#include "allocator_base.hpp"
#include "affix_allocator.hpp"
#include <chrono>

namespace alb {

  /// Use this macro if you want to store the caller information
#define ALLOCATE(A, N) A.allocate(N, __FILE__, __FUNCTION__, __LINE__)

  /// Simple way to define member and accessors
#define MEMBER_ACCESSOR(X)                                                                         \
private:                                                                                           \
  size_t X##_;                                                                                     \
                                                                                                   \
public:                                                                                            \
  size_t X() const noexcept                                                                         \
  {                                                                                                \
    return X##_;                                                                                   \
  }

  inline namespace v_100 {
    /**
    * The following options define what statistics shall be collected during runtime
    * taken from https://github.com/andralex/phobos/blob/allocator/std/allocator.d
    * and adapted to this implementation.
    *
    * \ingroup group_stats
    */
    enum StatsOptions : unsigned {
      /**
      * Counts the number of calls to alb::allocator_with_stats::owns.
      */
      NumOwns = 1u << 0,
      /**
      * Counts the number of calls to alb::allocator_with_stats::allocate.
      * All calls are counted, including requests for zero bytes or failed requests.
      */
      NumAllocate = 1u << 1,
      /**
      * Counts the number of calls to alb::allocator_with_stats::allocate
      * that succeeded, i.e. they wherefor more than zero bytes and returned a
      * non-null block.
      */
      NumAllocateOK = 1u << 2,
      /**
      * Counts the number of calls to alb::allocator_with_stats::expand, regardless
      * of arguments or result.
      */
      NumExpand = 1u << 3,
      /**
      * Counts the number of calls to alb::allocator_with_stats::expand that resulted
      * in a successful expansion.
      */
      NumExpandOK = 1u << 4,
      /**
      * Counts the number of calls to alb::allocator_with_stats::reallocate,
      * regardless of arguments or result.
      */
      NumReallocate = 1u << 5,
      /**
      * Counts the number of calls to alb::allocator_with_stats::reallocate that
      * succeeded. (Reallocations to zero bytes count as successful.)
      */
      NumReallocateOK = 1u << 6,
      /**
      * Counts the number of calls to alb::allocator_with_stats::reallocate that
      * resulted in an in-place reallocation (no memory moved). If this number
      * is close to the total number of reallocations, that indicates the allocator
      * finds room at the current block's end in a large fraction of the cases, but
      * also that internal fragmentation may be high (the size of the unit of
      * allocation is large compared to the typical allocation size of the
      * application).
      */
      NumReallocateInPlace = 1u << 7,
      /**
      * Counts the number of calls to alb::allocator_with_stats::deallocate.
      */
      NumDeallocate = 1u << 8,
      /**
      * Counts the number of calls to alb::allocator_with_stats::deallocate_all.
      */
      NumDeallocateAll = 1u << 9,
      /**
      * Chooses all numXxx)flags.
      */
      NumAll = (1u << 10) - 1,
      /**
      * Tracks total cumulative bytes allocated by means of
      * alb::allocator_with_stats::allocate, alb::allocator_with_stats::expand, and
      * alb::allocator_with_stats::reallocate (when resulting in an expansion). This
      * number always grows and indicates allocation traffic. To compute bytes
      * currently allocated, subtract alb::allocator_with_stats::bytesDeallocated
      * (below) from alb::allocator_with_stats::bytesAllocated.
      */
      BytesAllocated = 1u << 10,
      /**
      * Tracks total cumulative bytes deallocated by means of
      * alb::allocator_with_stats::deallocate and alb::allocator_with_stats::reallocate
      * (when resulting in a contraction). This number always grows and indicates
      * deallocation traffic.
      */
      BytesDeallocated = 1u << 11,
      /**
      * Tracks the sum of all delta values in calls of the form
      * alb::allocator_with_stats::expand(b, delta)) that succeed.
      */
      BytesExpanded = 1u << 12,
      /**
      * Tracks the sum of all (b.length - s) with (b.length > s) in calls of
      * the form alb::allocator_with_stats::reallocate(b, s)) that succeed.
      */
      BytesContracted = 1u << 13,
      /**
      * Tracks the sum of all bytes moved as a result of calls to
      * alb::allocator_with_stats::reallocate that
      * were unable to reallocate in place. A large number (relative to
      * bytesAllocated)) indicates that the application should use larger
      * preallocations.
      */
      BytesMoved = 1u << 14,
      /**
      * Measures the sum of extra bytes allocated beyond the bytes requested, i.e.
      * the http://goo.gl/YoKffF, internal fragmentation). This is the current
      * effective number of slack bytes, and it goes up and down with time.
      */
      BytesSlack = 1u << 15,
      /**
      * Measures the maximum bytes allocated over the time. This is useful for
      * dimensioning allocators.
      */
      BytesHighTide = 1u << 16,
      /**
      * Chooses all byteXxx flags.
      */
      BytesAll = ((1u << 17) - 1) & ~NumAll,
      /**
      * Instructs AllocatorWithStats to store the size asked by the caller for
      * each allocation. All per-allocation data is stored just before the actually
      * allocation by using an affix_allocator.
      */
      CallerSize = 1u << 17,
      /**
      * Instructs AllocatorWithStats to store the caller's file for each
      * allocation.
      */
      CallerFile = 1u << 18,
      /**
      * Instructs AllocatorWithStats to store the caller function name for
      * each allocation.
      */
      CallerFunction = 1u << 19,
      /**
      * Instructs AllocatorWithStats to store the caller's line for each
      * allocation.
      */
      CallerLine = 1u << 20,
      /**
      * Instructs AllocatorWithStats to store the time of each allocation.
      */
      CallerTime = 1u << 21,
      /**
      * Chooses all callerXxx flags.
      */
      CallerAll = ((1u << 22) - 1) & ~NumAll & ~BytesAll,
      /**
      * Combines all flags above.
      */
      All = (1u << 22) - 1
    };

    /**
    * This Allocator serves as a facade in front of the specified allocator to
    * collect statistics during runtime about all operations done on this instance.
    * This is an implementation that is not intended to be used in a shared
    * environment.
    *
    * In case that caller information shall be collected, the Allocator
    * parameter is encapsulated with an ALB::affix_allocator. In this case
    * alb::allocator_with_stats::AllocationInfo is in used as Prefix and so all
    * caller information is prepended to every allocated block.
    * Be aware that collecting of caller informations adds on top of each
    * allocation
    * sizeof(AllocatorWithStats::AllocationInfo) bytes!
    * With a good optimizing compiler only the code for the enabled
    * statistic information gets created.
    * \tparam Allocator The allocator that performs all allocations
    * \tparam Flags Specifies what kind of statistics get collected
    *
    * \ingroup group_allocators group_stats
    */
    template <class Allocator, unsigned Flags = alb::StatsOptions::All> class allocator_with_stats {
    public:
      /**
      * In case that we store allocation state, we use an affix_allocator to store
      * the additional informations as a Prefix
      *
      * \ingroup group_stats
      */
      struct AllocationInfo {
        size_t callerSize;
        const char *callerFile;
        const char *callerFunction;
        int callerLine;

        /* The comparison does not take the allocation time into account
        * It is a template to be able to compare the AllocationInfo from different allocators
        */
        template <typename RHS> bool operator==(const RHS &rhs) const
        {
          return callerSize == rhs.callerSize &&
            (callerFile == rhs.callerFile || ::strcmp(callerFile, rhs.callerFile) == 0) &&
            (callerFunction == rhs.callerFunction ||
              ::strcmp(callerFunction, rhs.callerFunction) == 0);
        }

        std::chrono::time_point<std::chrono::system_clock> callerTime;
        AllocationInfo *previous, *next;
      };

      /**
      * This container implements a facade over all currently available
      * AllocationInfo. The alb::allocator_with_stats owns all elements and changing
      * any element has undefined behavior!
      *
      * \ingroup group_stats
      */
      class Allocations {
      public:
        /**
        * Iterator for Allocations elements
        */
        class iterator {
        public:
          using iterator_category = std::bidirectional_iterator_tag;
          using value_type = AllocationInfo *;
          using difference_type = ptrdiff_t;
          using pointer = value_type;
          using const_pointer = const pointer;
          using reference = value_type;
          using const_reference = const reference;

        public:
          iterator()
            : _node{ nullptr }
          {}

          explicit iterator(AllocationInfo *data)
            : _node{ data }
          {}

          reference operator*() const {
            return _node;
          }

          pointer operator->() const {
            return &(operator*());
          }

          iterator &operator++() {
            _node = _node->next;
            return *this;
          }

          iterator operator++(int) {
            iterator tmp = *this;
            ++*this;
            return tmp;
          }

          iterator &operator--() {
            _node = _node->previous();
            return *this;
          }

          iterator operator--(int) {
            iterator tmp = *this;
            --*this;
            return tmp;
          }

          friend bool operator==(const iterator &x, const iterator &y) {
            return x._node == y._node;
          }

          friend bool operator!=(const iterator &x, const iterator &y) {
            return !(x == y);
          }

        private:
          AllocationInfo *_node;
        };

      public:
        using const_iterator = const iterator;

        explicit Allocations(AllocationInfo *root)
          : _begin(root)
          , _end(nullptr)
        {}

        const_iterator cbegin() const {
          return _begin;
        }

        const_iterator cend() const {
          return _end;
        }

        bool empty() const {
          return _begin == _end;
        }

      private:
        const const_iterator _begin;
        const const_iterator _end;
      };

      static const bool HasPerAllocationState =
        (Flags &
          (StatsOptions::CallerTime | StatsOptions::CallerFile | StatsOptions::CallerLine)) != 0;

      // Simplification for defining all members and accessors.
#define MEMBER_ACCESSORS                                                                           \
    MEMBER_ACCESSOR(num_owns)                                                                         \
    MEMBER_ACCESSOR(num_allocate)                                                                     \
    MEMBER_ACCESSOR(num_allocate_ok)                                                                   \
    MEMBER_ACCESSOR(num_expand)                                                                       \
    MEMBER_ACCESSOR(num_expand_ok)                                                                     \
    MEMBER_ACCESSOR(num_reallocate)                                                                   \
    MEMBER_ACCESSOR(num_reallocate_ok)                                                                 \
    MEMBER_ACCESSOR(num_reallocate_in_place)                                                            \
    MEMBER_ACCESSOR(num_deallocate)                                                                   \
    MEMBER_ACCESSOR(num_deallocate_all)                                                                \
    MEMBER_ACCESSOR(bytes_allocated)                                                                  \
    MEMBER_ACCESSOR(bytes_deallocated)                                                                \
    MEMBER_ACCESSOR(bytes_expanded)                                                                   \
    MEMBER_ACCESSOR(bytes_contracted)                                                                 \
    MEMBER_ACCESSOR(bytes_moved)                                                                      \
    MEMBER_ACCESSOR(bytes_slack)                                                                      \
    MEMBER_ACCESSOR(bytes_high_tide)

      MEMBER_ACCESSORS

#undef MEMBER_ACCESSOR
#undef MEMBER_ACCESSORS

        static constexpr bool supports_truncated_deallocation = Allocator::supports_truncated_deallocation;

      static constexpr bool has_per_allocation_state = HasPerAllocationState;

      allocator_with_stats() noexcept
        : num_owns_(0)
        , num_allocate_(0)
        , num_allocate_ok_(0)
        , num_expand_(0)
        , num_expand_ok_(0)
        , num_reallocate_(0)
        , num_reallocate_ok_(0)
        , num_reallocate_in_place_(0)
        , num_deallocate_(0)
        , num_deallocate_all_(0)
        , bytes_allocated_(0)
        , bytes_deallocated_(0)
        , bytes_expanded_(0)
        , bytes_contracted_(0)
        , bytes_moved_(0)
        , bytes_slack_(0)
        , bytes_high_tide_(0)
        , root_(nullptr)
      {
      }

      /**
      * The number of specified bytes gets allocated by the underlying Allocator.
      * Depending on the specified Flag, the allocating statistic information
      * is stored.
      * \param n The requested number of bytes
      * \param file The file name of the caller location (Only stored if CallerFile
      * is enabled)
      * \param function The callers function (Only stored if CallerFunction is
      * enabled)
      * \param line The callers line in source code (Only stored if CallerLine is
      * enabled)
      */
      block allocate(size_t n, const char *file = nullptr, const char *function = nullptr,
        int line = 0) noexcept
      {

        auto result = allocator_.allocate(n);
        up(StatsOptions::NumAllocate, num_allocate_);
        upOK(StatsOptions::NumAllocateOK, num_allocate_ok_, n > 0 && result);
        add(StatsOptions::BytesAllocated, bytes_allocated_, result.length);
        update_high_tide();

        if (has_per_allocation_state) {
          if (result) {
            AllocationInfo *stat =
              traits::affix_extractor<decltype(allocator_), AllocationInfo>::prefix(allocator_,
                result);

            set(StatsOptions::CallerSize, stat->callerSize, n);
            set(StatsOptions::CallerFile, stat->callerFile, file);
            set(StatsOptions::CallerFunction, stat->callerFunction, function);
            set(StatsOptions::CallerLine, stat->callerLine, line);
            set(StatsOptions::CallerTime, stat->callerTime, std::chrono::system_clock::now());

            // push into caller info stack
            if (root_) {
              root_->previous = stat;
              stat->next = root_;
              stat->previous = nullptr;
              root_ = stat;
            }
            else { // create the new stack
              stat->previous = nullptr;
              stat->next = nullptr;
              root_ = stat;
            }
          }
        }
        return result;
      }

      /**
      * The specified block gets freed by the underlaying Allocator
      * Depending on the specified Flag, the deallocating statistic information
      * is stored.
      * \param b Block to be freed
      */
      void deallocate(block &b) noexcept
      {
        up(StatsOptions::NumDeallocate, num_deallocate_);
        add(StatsOptions::BytesDeallocated, bytes_deallocated_, b.length);

        if (has_per_allocation_state) {
          if (b) {
            auto stat =
              traits::affix_extractor<decltype(allocator_), AllocationInfo>::prefix(allocator_, b);
            if (stat->previous) {
              stat->previous->next = stat->next;
            }
            if (stat->next) {
              stat->next->previous = stat->previous;
            }
            if (stat == root_) {
              root_ = stat->previous;
            }
          }
        }
        allocator_.deallocate(b);
      }

      /**
      * The specified block gets reallocated by the underlaying Allocator
      * Depending on the specified Flag, the reallocating statistic information
      * is stored.
      * \param b The block that should be reallocated.
      * \param n The new size. If zero, then a deallocation takes place
      * \return True, if the operation was successful
      */
      bool reallocate(block &b, size_t n) noexcept
      {
        auto originalBlock = b;
        auto wasRootBlock(false);
        if (has_per_allocation_state) {
          if (b) {
            wasRootBlock =
              root_ ==
              traits::affix_extractor<decltype(allocator_), AllocationInfo>::prefix(allocator_, b);
          }
        }
        up(StatsOptions::NumReallocate, num_reallocate_);

        if (!allocator_.reallocate(b, n)) {
          return false;
        }
        up(StatsOptions::NumReallocateOK, num_reallocate_ok_);
        std::make_signed<size_t>::type delta = b.length - originalBlock.length;
        if (b.ptr == originalBlock.ptr) {
          up(StatsOptions::NumReallocateInPlace, num_reallocate_in_place_);
          if (delta > 0) {
            add(StatsOptions::BytesAllocated, bytes_allocated_, delta);
            add(StatsOptions::BytesExpanded, bytes_expanded_, delta);
          }
          else {
            add(StatsOptions::BytesDeallocated, bytes_deallocated_, -delta);
            add(StatsOptions::BytesContracted, bytes_contracted_, -delta);
          }
        } // was moved to a new location
        else {
          add(StatsOptions::BytesAllocated, bytes_allocated_, b.length);
          add(StatsOptions::BytesMoved, bytes_moved_, originalBlock.length);
          add(StatsOptions::BytesDeallocated, bytes_deallocated_, originalBlock.length);

          if (has_per_allocation_state) {
            if (b) {
              auto stat =
                traits::affix_extractor<decltype(allocator_), AllocationInfo>::prefix(allocator_, b);
              if (stat->next) {
                stat->next->previous = stat;
              }
              if (stat->previous) {
                stat->previous->next = stat;
              }
              if (wasRootBlock) {
                root_ = stat;
              }
            }
          }
        }
        update_high_tide();
        return true;
      }

      /**
      * The given block is passed to the underlying Allocator to be checked
      * for ownership.
      * This method is only available if the underlying Allocator implements it.
      * Depending on the specified Flag, only the owns statistic information
      * is stored.
      * \param b The block its ownership shall be checked
      */
      template <typename U = Allocator>
      typename std::enable_if<traits::has_owns<U>::value, bool>::type
        owns(const block &b) const noexcept
      {
        up(StatsOptions::NumOwns, num_owns_);
        return allocator_.owns(b);
      }

      /**
      * The given block is passed to the underlying Allocator to be expanded
      * This method is only available if the underlying allocator implements it.
      * Depending on the specified Flag, the expand statistic information
      * is stored.
      * \param b The block that should be expanded
      * \param delta The amount of bytes that should be tried to expanded
      * \return True, if the operation was successful
      */
      template <typename U = Allocator>
      typename std::enable_if<traits::has_expand<U>::value, bool>::type
        expand(block &b, size_t delta) noexcept
      {
        up(StatsOptions::NumExpand, num_expand_);
        auto oldLength = b.length;
        auto result = allocator_.expand(b, delta);
        if (result) {
          up(StatsOptions::NumExpandOK, num_expand_ok_);
          add(StatsOptions::BytesExpanded, bytes_expanded_, b.length - oldLength);
          add(StatsOptions::BytesAllocated, bytes_allocated_, b.length - oldLength);
          update_high_tide();
          // if (b && has_per_allocation_state) {
          //   auto stat = traits::AffixExtractor<
          //       decltype(allocator_), AllocationInfo>::prefix(allocator_, b);
          // }
        }
        return result;
      }

      /**
      * Accessor to all currently outstanding memory allocations. The ownership
      * of all elements belong to this class.
      * \return A container with all AllocationInfos
      */
      Allocations allocations() const noexcept
      {
        return Allocations(root_);
      }

    private:
      /**
      * Increases the given value by one if the passed option is set
      */
      template <typename T> inline void up(StatsOptions option, T &value) noexcept
      {
        if (Flags & option)
          ++value;
      }

      /**
      * Increases the given value by one if the passed option is set and the bool
      * is set to true
      */
      template <typename T> inline void upOK(StatsOptions option, T &value, bool ok) noexcept
      {
        if (Flags & option && ok)
          ++value;
      }

      /**
      * Adds the given delta value to the passed value, if the given option is
      * set. Delta can be negative
      */
      template <typename T>
      void inline add(StatsOptions option, T &value, typename std::make_signed<T>::type delta) noexcept
      {
        if (Flags & option)
          value += delta;
      }

      /**
      * Sets the given value to the passed reference, if the passed option is set
      */
      template <typename T> void set(StatsOptions option, T &value, T t) noexcept
      {
        if (Flags & option)
          value = std::move(t);
      }

      /**
      * If the high tide information shall be collected, it is recalculated
      */
      void update_high_tide() noexcept
      {
        if (Flags & StatsOptions::BytesHighTide) {
          const size_t currentlyAllocated = bytes_allocated_ - bytes_deallocated_;
          if (bytes_high_tide_ < currentlyAllocated) {
            bytes_high_tide_ = currentlyAllocated;
          }
        }
      }

      /**
      * Depending on setting that caller information shall be collected
      * an affix_allocator or the specified Allocator directly is used.
      */
      typename traits::type_switch<affix_allocator<Allocator, AllocationInfo>, Allocator,
        HasPerAllocationState>::type allocator_;

      AllocationInfo *root_;
    };
  }
  using namespace v_100;
}