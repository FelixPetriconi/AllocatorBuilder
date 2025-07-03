///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_DYNASTIC_HPP
#define ALB_DYNASTIC_HPP

#include <cstdint>
#include <limits>

namespace alb {
inline namespace v_100 {
namespace internal {

/**
 * Flag to be used inside the Dynastic struct to signal that the value
 * can be changed during runtime.
 * \ingroup group_internal
 */
enum class DynasticOptions : std::size_t {
    DynasticUndefined = std::numeric_limits<std::size_t>::max(),
    DynasticDynamicSet = std::numeric_limits<std::size_t>::max() - 1
};

/**
 * Simple generic value type that is either compile time constant or dynamically
 * set-able depending of DynamicEnableSwitch. If v and DynamicEnableSwitch, then
 * value can be changed during runtime.
 * @Author Andrei Alexandrescu
 *
 * \ingroup group_internal
 */
template <std::size_t v, std::size_t DynamicEnableSwitch>
struct dynastic {
    constexpr std::size_t value() const noexcept { return v; }
};

template <size_t DynamicEnableSwitch>
struct dynastic<DynamicEnableSwitch, DynamicEnableSwitch> {
private:
    std::size_t v_;

public:
    constexpr dynastic() noexcept : v_(static_cast<std::size_t>(DynasticOptions::DynasticUndefined)) {}

    constexpr std::size_t value() const noexcept { return v_; }

    constexpr void value(std::size_t w) noexcept { v_ = w; }
};

} // namespace internal
} // namespace v_100

using namespace v_100;
} // namespace alb

#endif