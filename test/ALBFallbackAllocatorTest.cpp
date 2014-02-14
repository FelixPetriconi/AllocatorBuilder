///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi 
//
///////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include "ALBFallbackAllocator.h"
#include "ALBTestHelpers.h"
#include "ALBStackAllocator.h"
#include "ALBSharedHeap.h"
#include "ALBMallocator.h"

class FallbackAllocatorTest : public 
  ALB::TestHelpers::AllocatorBaseTest<
    ALB::FallbackAllocator<
      ALB::StackAllocator<32>, 
        ALB::SharedHeap<ALB::Mallocator,512,4>>>
{};


// TODO