///////////////////////////////////////////////////////////////////
//
// Copyright 2015 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
//////////////////////////////////////////////////////////////////
#ifndef ALB_NULL_ALLOCATOR_HPP
#define ALB_NULL_ALLOCATOR_HPP

#include <alb/allocator_base.hpp>
#include <alb/config.hpp>

#include <cassert>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {

class null_allocator {
public:
    static const std::uint32_t alignment = 64 * 1024;

    block allocate(std::size_t) noexcept { return {nullptr, 0}; }

    bool owns(const block& b) noexcept { return !b; }

    bool expand(block& b, std::size_t) noexcept {
        assert(!b);
        return false;
    }

    bool reallocate(block& b, std::size_t) noexcept {
        assert(!b);
        return false;
    }

    void deallocate(block& b) noexcept { assert(!b); }

    void deallocate_all() noexcept {}
};
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif