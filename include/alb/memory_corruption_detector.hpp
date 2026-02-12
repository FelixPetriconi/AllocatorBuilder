///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_MEMORY_CORRUPTION_DETECTOR_HPP
#define ALB_MEMORY_CORRUPTION_DETECTOR_HPP

#include <alb/config.hpp>

#include <cassert>
#include <cstdint>
#include <exception>
#include <iostream>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
/**
 * This class can be used as Prefix and/or Suffix with the affix_allocator to
 * detect
 * buffer under-runs or overflows.
 * \tparam T must be an integral type. A guard with its size is used
 * \tparam Pattern this pattern is put into the memory as guard e.g. 0xdeadbeef
 *
 * \ingroup group_internal
 */
template <typename T, std::size_t Pattern>
class memory_corruption_detector {
    static_assert(sizeof(char) < sizeof(T) && sizeof(T) <= sizeof(std::uint64_t),
                  "Memory check not for supported types");

    T pattern_;

public:
    using value_type = T;
    static const std::size_t pattern = Pattern;

    memory_corruption_detector() noexcept : pattern_(Pattern) {}

    ~memory_corruption_detector() {
#ifndef NDEBUG
        if (pattern_ != Pattern) {
            std::cout << std::to_string(pattern_) << " is unequal to " << std::to_string(Pattern)
                      << "!" << std::endl;
            std::terminate();
        }
#endif
        assert(pattern_ == Pattern);
    }
};

template <typename T, size_t Pattern>
const std::size_t memory_corruption_detector<T, Pattern>::pattern;
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif