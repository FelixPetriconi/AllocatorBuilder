///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_STACK_HPP
#define ALB_STACK_HPP

#include <type_traits>

namespace alb {
inline namespace v_100 {
namespace internal {

/**
 * Simple stack with the same interface as boost::lockfree::stack with a fixed
 * number of elements
 * It's intend is to be used as not shared variant in the FreeList
 * \tparam T The element type to be put into the stack
 * \tparam MaxSize The maximum number of elements that can be put into the stack
 *
 * \ingroup group_internal
 */
template <typename T, unsigned MaxSize>
class stack {
    static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
    static_assert(std::is_trivially_destructible_v<T>, "T must be trivially destroyable");

    T elements_[MaxSize] = {};
    std::int32_t pos_;

public:
    using value_type = T;
    static const size_t max_size = MaxSize;

    stack() noexcept : pos_(-1) {}

    bool push(T v) noexcept {
        if (pos_ < static_cast<int>(MaxSize) - 1) {
            pos_++;
            elements_[pos_] = v;
            return true;
        }
        return false;
    }

    bool pop(T& v) noexcept {
        if (pos_ >= 0) {
            v = elements_[pos_];
            pos_--;
            return true;
        }
        return false;
    }

    bool empty() const noexcept { return pos_ == -1; }
};
} // namespace internal
} // namespace v_100

using namespace v_100;
} // namespace alb

#endif