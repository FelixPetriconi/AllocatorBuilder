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

#include <alb/config.hpp>

#include <shared_mutex>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
namespace internal {

/**
 * Class that does not lock a given mutex
 *
 * \ingroup group_internal
 */
class null_lock {
public:
    [[nodiscard]] explicit null_lock(std::shared_mutex&) noexcept {}
};

/**
 * Class that locks with a shared lock then given mutex
 *
 * \ingroup group_internal
 */
class shared_lock {
    std::shared_lock<std::shared_mutex> lock_;

public:
    [[nodiscard]] explicit shared_lock(std::shared_mutex& m) noexcept : lock_(m) {}
};

/**
 * Class that locks with a unique lock a given mutex
 *
 * \ingroup group_internal
 */
class unique_lock {
    std::unique_lock<std::shared_mutex> lock_;

public:
    [[nodiscard]] explicit unique_lock(std::shared_mutex& m) noexcept : lock_(m) {}
};

struct null_mutex {};

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

} // namespace internal
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif