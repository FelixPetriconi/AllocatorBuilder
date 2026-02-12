///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_HEAP_HELPERS_HPP
#define ALB_HEAP_HELPERS_HPP

#include <alb/config.hpp>
#include <cstdint>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
namespace internal {

template <bool Used>
std::uint64_t set_used(std::uint64_t const& currentRegister, std::uint64_t const& mask) noexcept;

template <>
inline std::uint64_t set_used<false>(std::uint64_t const& currentRegister,
                                     std::uint64_t const& mask) noexcept {
    return currentRegister & (mask ^ std::uint64_t(-1));
}
template <>
inline std::uint64_t set_used<true>(std::uint64_t const& currentRegister,
                                    std::uint64_t const& mask) noexcept {
    return currentRegister | mask;
}
} // namespace internal
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif