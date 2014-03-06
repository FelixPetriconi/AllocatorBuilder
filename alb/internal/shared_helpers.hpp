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

#include <boost/thread.hpp>
#include <mutex>

namespace alb {
namespace shared_helpers {

/**
 * Class that does not lock a given mutex
 *
 * \ingroup group_internal
 */
class NullLock
{
public:
  NullLock(boost::shared_mutex&) {}
};

/**
 * Class that locks with a shared lock then given mutex
 *
 * \ingroup group_internal
 */
class SharedLock
{
  boost::shared_lock<boost::shared_mutex> _lock;
public:
  SharedLock(boost::shared_mutex& m) : _lock(m) {}
};

/**
 * Class that locks with a unique lock a given mutex
 *
 * \ingroup group_internal
 */
class UniqueLock
{
  boost::unique_lock<boost::shared_mutex> _lock;
public:
  UniqueLock(boost::shared_mutex& m) : _lock(m) {}
};

struct null_mutex {};

struct null_lock {
  null_lock(null_mutex&) {}
};

template <class M>
struct lock_guard;

template <>
struct lock_guard<null_mutex> {
  lock_guard(null_mutex&) {}
};

template <>
struct lock_guard<std::mutex> {
  std::unique_lock<std::mutex> _lock;
private:
  lock_guard(std::mutex& m) : _lock(m) {}
};


} // namespace helpers
} // namespace alb
