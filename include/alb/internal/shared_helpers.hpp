///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_SHARED_HELPERS_HPP
#define ALB_SHARED_HELPERS_HPP

#include <shared_mutex>

namespace alb {
inline namespace v_100 {
namespace shared_helpers {

/**
 * Class that does not lock a given mutex
 *
 * \ingroup group_internal
 */
class NullLock {
public:
    [[nodiscard]] explicit NullLock(std::shared_mutex&) noexcept {}
};

/**
 * Class that locks with a shared lock then given mutex
 *
 * \ingroup group_internal
 */
class SharedLock {
    std::shared_lock<std::shared_mutex> _lock;

public:
    [[nodiscard]] explicit SharedLock(std::shared_mutex& m) noexcept : _lock(m) {}
};

/**
 * Class that locks with a unique lock a given mutex
 *
 * \ingroup group_internal
 */
class UniqueLock {
    std::unique_lock<std::shared_mutex> _lock;

public:
    [[nodiscard]] explicit UniqueLock(std::shared_mutex& m) noexcept : _lock(m) {}
};

struct null_mutex {};

struct null_lock {
    explicit null_lock(null_mutex&) noexcept {}
};

template <class M>
struct lock_guard;

template <>
struct lock_guard<null_mutex> {
    [[nodiscard]] lock_guard(null_mutex&) noexcept {}
};

template <>
struct lock_guard<std::mutex> {
    std::unique_lock<std::mutex> _lock;

private:
    explicit lock_guard(std::mutex& m) noexcept : _lock(m) {}
};

} // namespace shared_helpers
} // namespace v_100
using namespace v_100;
} // namespace alb

#endif