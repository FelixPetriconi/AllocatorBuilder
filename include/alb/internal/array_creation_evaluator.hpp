///////////////////////////////////////////////////////////////////
//
// Copyright 2015 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#ifndef ALB_ARRAY_CREATION_EVALUATOR_HPP
#define ALB_ARRAY_CREATION_EVALUATOR_HPP

#include <alb/config.hpp>

#include <cstddef>

namespace alb {

inline namespace ALB_VERSION_NAMESPACE() {

namespace internal {

std::size_t array_offset();

}
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif