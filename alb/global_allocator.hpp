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

namespace alb {
  struct length_prefix {
    unsigned length;
  };

  template <class Allocator> class global_allocator {
  public:
    using value_type = Allocator;

    static Allocator &instance()
    {
      static Allocator in;
      return in;
    }
  };
}
