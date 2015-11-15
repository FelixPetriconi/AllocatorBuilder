///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////

#include "array_creation_evaluator.hpp"
#include <new>
#include <stddef.h>

namespace
{
  struct Test
  {
    Test() : v(0)
    {
      s[0] = '\0';
    }
    
    ~Test()
    {
      s[0] = '\0';
    }

    int v;
    char s[2];
  };

  size_t array_offset_internal()
  {
    alignas(Test) char buffer[sizeof(Test) * 6];
    Test* p = new (buffer) Test[5];

    return reinterpret_cast<char*>(p) - buffer;
  }
}

 
size_t alb::v_100::helpers::array_offset()
{
  static size_t result = array_offset_internal();
  return result;
}