///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
///////////////////////////////////////////////////////////////////
#include "ALBTestHelpers.h"
#include <gtest/gtest.h>

void ALB::TestHelpers::EXPECT_MEM_EQ(void* a, void* b, size_t n)
{
  auto isDifferentAt = ::memcmp(a, b, n);
  if (isDifferentAt)
  {
    int i = 0;
    (void)i;
  }
  EXPECT_EQ(0, isDifferentAt) 
    << "Memory is different at offset: " 
    << isDifferentAt 
    << ". Should be " << (int)*(static_cast<char*>(b) + isDifferentAt) 
    << " but should be " << (int)*(static_cast<char*>(a) + isDifferentAt);
}

std::atomic<size_t> ALB::TestHelpers::TestMallocator::_allocatedMem = 0;
