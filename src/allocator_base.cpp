///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#include <alb/allocator_base.hpp>

#include <algorithm>
#include <cstring>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
namespace internal {
void block_copy(const alb::block& source, alb::block& destination) noexcept {
    ::memcpy(destination.ptr, source.ptr, std::min(source.length, destination.length));
}
} // namespace internal
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb