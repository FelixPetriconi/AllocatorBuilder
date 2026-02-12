///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_GLOBAL_ALLOCATOR_HPP
#define ALB_GLOBAL_ALLOCATOR_HPP

#include <alb/config.hpp>

namespace alb {

inline namespace ALB_VERSION_NAMESPACE() {

struct length_prefix {
    std::uint32_t length;
};

template <class Allocator>
class global_allocator {
public:
    using value_type = Allocator;

    static Allocator& instance() {
        static Allocator in;
        return in;
    }
};
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif