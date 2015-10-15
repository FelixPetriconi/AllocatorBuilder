///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
///////////////////////////////////////////////////////////////////
#include "allocator_base.hpp"

#include <cstring>
#include <algorithm>

void alb::internal::blockCopy(const alb::block& source, alb::block& destination) noexcept
{
  ::memcpy(destination.ptr, source.ptr, std::min(source.length, destination.length));
}