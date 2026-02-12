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

#include <alb/config.hpp>

#include <cstdint>
#include <limits>

namespace alb {

inline namespace ALB_VERSION_NAMESPACE() {

namespace internal {

/**
 * Flag to be used inside the Dynastic struct to signal that the value
 * can be changed during runtime.
 * \ingroup group_internal
 */
enum class dynastic_options : std::size_t {
    undefined = std::numeric_limits<std::size_t>::max(),
    dynastic_dynamic_set = std::numeric_limits<std::size_t>::max() - 1
};

/**
 * Simple generic value type that is either compile time constant or dynamically
 * set-able depending on DynamicEnableSwitch. If v and DynamicEnableSwitch, then
 * value can be changed during runtime.
 * @Author Andrei Alexandrescu
 *
 * \ingroup group_internal
 */
template <std::size_t v, std::size_t DynamicEnableSwitch>
struct dynastic {
    constexpr std::size_t value() const noexcept { return v; }
};

template <std::size_t DynamicEnableSwitch>
struct dynastic<DynamicEnableSwitch, DynamicEnableSwitch> {
private:
    std::size_t v_;

public:
    constexpr dynastic() noexcept : v_(static_cast<std::size_t>(dynastic_options::undefined)) {}

    constexpr std::size_t value() const noexcept { return v_; }

    constexpr void value(std::size_t w) noexcept { v_ = w; }
};

} // namespace internal
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif