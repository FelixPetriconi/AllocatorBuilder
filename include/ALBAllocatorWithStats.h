///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: 
//          http://erdani.com, Andrei Alexandrescu
//          http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#pragma once

#include "ALBAllocatorBase.h"

namespace ALB {

  /**
   * The following options define what statistics shall be collected during runtime
   * Directly taken from https://github.com/andralex/phobos/blob/allocator/std/allocator.d
   */
  enum StatsOptions : unsigned { 
    /**
     * Counts the number of calls to @see #owns).
     */
    numOwns = 1u << 0,
    /**
     * Counts the number of calls to @see #allocate. All calls are counted,
     * including requests for zero bytes or failed requests.
    */
    numAllocate = 1u << 1,
    /**
     * Counts the number of calls to @see #allocate that succeeded, i.e. they were
     * for more than zero bytes and returned a non-null block.
     */
    numAllocateOK = 1u << 2,
    /**
     * Counts the number of calls to @see #expand, regardless of arguments or
     * result.
     */
    numExpand = 1u << 3,
    /**
     * Counts the number of calls to @see #expand that resulted in a successful
     * expansion.
     */
    numExpandOK = 1u << 4,
    /**
     * Counts the number of calls to @see #reallocate, regardless of arguments or
     * result.
     */
    numReallocate = 1u << 5,
    /**
     * Counts the number of calls to @see #reallocate that succeeded. (Reallocations
     * to zero bytes count as successful.)
     */
    numReallocateOK = 1u << 6,
    /**
     * Counts the number of calls to @see #reallocate that resulted in an in-place
     * reallocation (no memory moved). If this number is close to the total number
     * of reallocations, that indicates the allocator finds room at the current
     * block's end in a large fraction of the cases, but also that internal
     * fragmentation may be high (the size of the unit of allocation is large
     * compared to the typical allocation size of the application).
     */
    numReallocateInPlace = 1u << 7,
    /**
     * Counts the number of calls to @see #deallocate.
     */
    numDeallocate = 1u << 8,
    /**
     * Counts the number of calls to @see #deallocateAll.
     */
    numDeallocateAll = 1u << 9,
    /**
     * Chooses all numXxx)flags.
     */
    numAll = (1u << 10) - 1,
    /**
     * Tracks total cumulative bytes allocated by means of @see #allocate,
     * @see #expand, and @see #reallocate (when resulting in an expansion). This
     * number always grows and indicates allocation traffic. To compute bytes
     * currently allocated, subtract @see #bytesDeallocated (below) from
     * @see #bytesAllocated.
     */
    bytesAllocated = 1u << 10,
    /**
     * Tracks total cumulative bytes deallocated by means of @see #deallocate and
     * @see #reallocate (when resulting in a contraction). This number always grows
     * and indicates deallocation traffic.
     */
    bytesDeallocated = 1u << 11,
    /**
     * Tracks the sum of all @delta values in calls of the form
     * @see #expand(b, delta)) that succeed (return @return).
     */
    bytesExpanded = 1u << 12,
    /**
     * Tracks the sum of all (b.length - s) with (b.length > s) in calls of
     * the form @see #reallocate(b, s)) that succeed (return $(D true)).
     */
    bytesContracted = 1u << 13,
    /**
     * Tracks the sum of all bytes moved as a result of calls to @see @reallocate that
     * were unable to reallocate in place. A large number (relative to 
     * bytesAllocated)) indicates that the application should use larger
     * preallocations.
     */
    bytesMoved = 1u << 14,
    /**
     * Measures the sum of extra bytes allocated beyond the bytes requested, i.e.
     * the $(WEB goo.gl/YoKffF, internal fragmentation). This is the current
     * effective number of slack bytes, and it goes up and down with time.
     */
    bytesSlack = 1u << 15,
    /**
     * Measures the maximum bytes allocated over the time. This is useful for
     * dimensioning allocators.
     */
    bytesHighTide = 1u << 16,
    /**
     * Chooses all byteXxx flags.
     */
    bytesAll = ((1u << 17) - 1) & ~numAll,
    /**
     * Instructs @see AllocatorWithStats to store the size asked by the caller for
     * each allocation. All per-allocation data is stored just before the actually
     * allocation @see AffixAllocator.
    */
    callerSize = 1u << 17,
    /**
     * Instructs @see AllocatorWithStats to store the caller module for each
     * allocation.
     */
    callerModule = 1u << 18,
    /**
     * Instructs @see AllocatorWithStats to store the caller's file for each
     * allocation.
     */
    callerFile = 1u << 19,
    /**
     * Instructs @see AllocatorWithStats to store the caller __FUNCTION__ for
     * each allocation.
     */
    callerFunction = 1u << 20,
    /**
     * Instructs @see AllocatorWithStats to store the caller's line for each
     * allocation.
     */
    callerLine = 1u << 21,
    /**
     * Instructs @see AllocatorWithStats to store the time of each allocation.
     */
    callerTime = 1u << 22,
    /**
     * Chooses all callerXxx flags.
     */
    callerAll = ((1u << 23) - 1) & ~numAll & ~bytesAll,
    /**
     * Combines all flags above.
     */
    all = (1u << 23) - 1      
  };


#define MEMBER_ACCESSOR(X)          \
    private:                        \
      size_t  _##X;                 \
    public:                         \
      size_t X() const { return _##X; }


  /**
   * This Allocator serves as a facade in front of the specified allocator to collect
   * statistics during runtime about all operations done on this instance.
   * Be aware that collecting of caller informations can easily consume lot's
   * of memory! With a good optimizing compiler only the code for the enabled
   * statistic information gets created.
   * @tparam Allocator The allocator that performs all allocations
   * @tparam Flags Specifies what kind of statistics get collected
   */
  template <class Allocator, unsigned Flags = ALB::StatsOptions::all>
  class AllocatorWithStats {
    Allocator _allocator;

#define MEMBER_ACCESSORS                   \
    MEMBER_ACCESSOR(numOwns)               \
    MEMBER_ACCESSOR(numAllocate)           \
    MEMBER_ACCESSOR(numAllocateOK)         \
    MEMBER_ACCESSOR(numExpand)             \
    MEMBER_ACCESSOR(numExpandOK)           \
    MEMBER_ACCESSOR(numReallocate)         \
    MEMBER_ACCESSOR(numReallocateOK)       \
    MEMBER_ACCESSOR(numReallocateInPlace)  \
    MEMBER_ACCESSOR(numDeallocate)         \
    MEMBER_ACCESSOR(numDeallocateAll)      \
    MEMBER_ACCESSOR(bytesAllocated)        \
    MEMBER_ACCESSOR(bytesDeallocated)      \
    MEMBER_ACCESSOR(bytesExpanded)         \
    MEMBER_ACCESSOR(bytesContracted)       \
    MEMBER_ACCESSOR(bytesMoved)            \
    MEMBER_ACCESSOR(bytesSlack)            \
    MEMBER_ACCESSOR(bytesHighTide)


  MEMBER_ACCESSORS

#undef MEMBER_ACCESSOR
#undef MEMBER_ACCESSORS

    template <typename T>
    void up(ALB::StatsOptions option, T& value) {
      if (Flags & option)
        value++;
    }
    template <typename T>
    void upOK(ALB::StatsOptions option, T& value, bool ok) {
      if (Flags & option && ok)
        value++;
    }
    template <typename T>
    void add(ALB::StatsOptions option, T& value, typename std::make_signed<T>::type delta) {
      if (Flags & option)
        value += delta;
    }

    void updateHighTide() {
      if (Flags & StatsOptions::bytesHighTide)
      {
        const size_t currentlyAllocated = _bytesAllocated - _bytesDeallocated;
        if (_bytesHighTide < currentlyAllocated) {
          _bytesHighTide = currentlyAllocated;
        }
      }
    }

  public:
    AllocatorWithStats() 
      : _numOwns(0)
      , _numAllocate(0)
      , _numAllocateOK(0)
      , _numExpand(0)
      , _numExpandOK(0)
      , _numReallocate(0)
      , _numReallocateOK(0)
      , _numReallocateInPlace(0)
      , _numDeallocate(0)
      , _numDeallocateAll(0)
      , _bytesAllocated(0)
      , _bytesDeallocated(0)
      , _bytesExpanded(0)
      , _bytesContracted(0)
      , _bytesMoved(0)
      , _bytesSlack(0)
      , _bytesHighTide(0)
    {}

    /**
     * The number of specified bytes gets allocated by the underlying Allocator.
     * Depending on the specified @see Flag, the allocating statistic information 
     * is stored.
     */
    Block allocate(size_t n) {
      auto result = _allocator.allocate(n);
      up(StatsOptions::numAllocate, _numAllocate);
      upOK(StatsOptions::numAllocateOK, _numAllocateOK, n > 0 && result);
      add(StatsOptions::bytesAllocated, _bytesAllocated, result.length);
      updateHighTide();
      return result;
    }

    /**
     * The specified block gets freed by the underlaying Allocator
     * Depending on the specified @see Flag, the deallocating statistic information
     * is stored.
     */
    void deallocate(Block& b) {
      up(StatsOptions::numDeallocate, _numDeallocate);
      add(StatsOptions::bytesDeallocated, _bytesDeallocated, b.length);
      _allocator.deallocate(b);
    }

    /**
     * The specified block gets reallocated by the underlaying Allocator
     * Depending on the specified @see Flag, the reallocating statistic information 
     * is stored.
     */
    bool reallocate(Block& b, size_t n) {
      auto originalBlock = b;
      up(StatsOptions::numReallocate, _numReallocate);

      if (!_allocator.reallocate(b, n))
      {
        return false;
      }
      up(StatsOptions::numReallocateOK, _numReallocateOK);
      std::make_signed<size_t>::type delta = b.length - originalBlock.length;
      if (b.ptr == originalBlock.ptr) {
        up(StatsOptions::numReallocateInPlace, _numReallocateInPlace);
        if (delta > 0) {
          add(StatsOptions::bytesAllocated, _bytesAllocated, delta);
          add(StatsOptions::bytesExpanded, _bytesExpanded, delta);
        }
        else {
          add(StatsOptions::bytesDeallocated, _bytesDeallocated, -delta);
          add(StatsOptions::bytesContracted, _bytesContracted, -delta);
        }
      }  // was moved to a new location
      else {
        add(StatsOptions::bytesAllocated, _bytesAllocated, b.length);
        add(StatsOptions::bytesMoved, _bytesMoved, originalBlock.length);
        add(StatsOptions::bytesDeallocated, _bytesDeallocated, originalBlock.length);
      }
      updateHighTide();
      return true;
    }

    /**
     * The given block is passed to the underlying Allocator to be checked
     * for ownership. 
     * This method is only available if the underlying Allocator implements it.
     * Depending on the specified @see Flag, the ownc statistic information 
     * is stored.
     */
    typename Traits::enable_result_to<bool, Traits::has_owns<Allocator>::value>::type 
    owns(const Block& b) const {
      up(StatsOptions::numOwns, _numOwns);
      return _allocator.owns(b);
    }

    /**
     * The given block is passed to the underlying Allocator to be expanded
     * This method is only available if the underlying allocator implements it.
     * Depending on the specified @see Flag, the expand statistic information 
     * is stored.
     */
    typename Traits::enable_result_to<bool, Traits::has_expand<Allocator>::value>::type 
    expand(Block& b, size_t delta) {
      up(StatsOptions::numExpand, _numExpand);
      auto oldLength = b.length;
      auto result = _allocator.expand(b, delta);
      if (result)
      {
        up(StatsOptions::numExpandOK, _numExpandOK);
        add(StatsOptions::bytesExpanded, _bytesExpanded, b.length - oldLength);
        add(StatsOptions::bytesAllocated, _bytesAllocated, b.length - oldLength);
        updateHighTide();
      }
      return result;
    }
  };
}