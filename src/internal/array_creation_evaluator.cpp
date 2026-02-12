///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////

#include <alb/internal/array_creation_evaluator.hpp>

#include <cstdint>
#include <new>

namespace {
struct Test {
    Test() = default;
    Test(const Test&) = default;
    Test& operator=(const Test&) = default;
    Test(Test&&) noexcept = default;
    Test& operator=(Test&&) noexcept = default;

    ~Test() { s[0] = '\0'; }

    std::int32_t v{};
    char s[2] = {};
};

std::size_t array_offset_internal() {
    alignas(Test) char buffer[sizeof(Test) * 6];
    Test* p = new (buffer) Test[5];

    return reinterpret_cast<char*>(p) - buffer;
}
} // namespace

std::size_t alb::internal::array_offset() {
    static std::size_t result = array_offset_internal();
    return result;
}