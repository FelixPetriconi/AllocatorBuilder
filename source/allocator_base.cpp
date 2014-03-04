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

void ALB::Helper::blockCopy(const ALB::Block& source, ALB::Block& destination)
{
  ::memcpy(destination.ptr, source.ptr, std::min(source.length, destination.length));
}