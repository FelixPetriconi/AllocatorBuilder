///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_ALLOCATOR_BASE_HPP
#define ALB_ALLOCATOR_BASE_HPP

#include <alb/config.hpp>
#include <alb/internal/array_creation_evaluator.hpp>
#include <alb/internal/traits.hpp>
#include <alb/stl_allocator_adapter.hpp>

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace alb {

inline namespace ALB_VERSION_NAMESPACE() {

template <class T, class Allocator, class... Args>
std::shared_ptr<T> make_shared(const Allocator& alloc, Args&&... args) {
    auto local_allocator = std_allocator_adapter<T, Allocator>(alloc);
    return std::allocate_shared<T>(local_allocator, std::forward<Args>(args)...);
}

/**
 * A general purpose deleter that implements the deleter interface used in
 * std::unique_ptr
 * Precondition: The Allocator must be an instance of an affix_allocator
 */
template <typename Allocator>
class deleter {
public:
    using allocator = Allocator;

private:
    allocator* allocator_{};
    std::size_t extend_{};

public:
    deleter() : allocator_(nullptr), extend_(1) {}

    explicit deleter(allocator& a) : allocator_(a) {}

    void set_allocator(allocator& a) { allocator_ = &a; }

    void set_extend(std::size_t e) { extend_ = e; }

    template <typename U>
    void operator()(U* p) {
        if (p) {
            // in case that p is an array of U, all objects must be destroyed
            for (auto i = 0u; i < extend_; ++i) {
                p[i].~U();
            }
            // The C++ standard allows that the allocated memory position is different to the
            // pointer of the first element. In this case
            auto real_memory_location = (extend_ == 1 || std::is_trivially_destructible_v<U>) ?
                                            p :
                                            (static_cast<void*>(reinterpret_cast<std::byte*>(p) -
                                                                internal::array_offset()));

            block pseudo_block(real_memory_location, 1);
            auto size_prefix = allocator_->outer_to_prefix(pseudo_block);
            block real_block(real_memory_location, *size_prefix);
            allocator_->deallocate(real_block);
        }
    }
};

template <class T, typename Allocator>
struct _unique_if {
    using _single_object = std::unique_ptr<T, deleter<Allocator>>;
};

template <class T, typename Allocator>
struct _unique_if<T[], Allocator> {
    using _unknown_bound = std::unique_ptr<T[], deleter<Allocator>>;
};

template <class T, size_t N, typename Allocator>
struct _unique_if<T[N], Allocator> {
    using _known_bound = void;
};

template <class T, class Allocator, class... Args>
typename _unique_if<T, Allocator>::_single_object make_unique(Allocator& a, Args&&... args) {
    auto b = a.allocate(sizeof(T));
    if (b) {
        auto p = a.outer_to_prefix(b);
        *p = static_cast<typename Allocator::prefix>(b.length);
        auto result =
            std::unique_ptr<T, deleter<Allocator>>(new (b.ptr) T(std::forward<Args>(args)...));
        result.get_deleter().set_allocator(a);
        return result;
    }
    return std::unique_ptr<T, deleter<Allocator>>();
}

template <class T, class Allocator>
typename _unique_if<T, Allocator>::_unknown_bound make_unique(Allocator& a, size_t n) {
    using t_no_extent = typename std::remove_extent_t<T>;

    auto b = a.allocate(sizeof(t_no_extent) * n + internal::array_offset());
    if (b) {
        auto p = a.outer_to_prefix(b);
        *p = static_cast<typename Allocator::prefix>(b.length);
        auto result =
            std::unique_ptr<t_no_extent[], deleter<Allocator>>(new (b.ptr) t_no_extent[n]);
        result.get_deleter().set_allocator(a);
        result.get_deleter().set_extend(n);
        return result;
    }
    return std::unique_ptr<T, deleter<Allocator>>();
}

template <class T, class Allocator, class... Args>
typename _unique_if<T, deleter<Allocator>>::_known_bound make_unique(Allocator&,
                                                                     Args&&...) = delete;

namespace internal {

/**
 * Copies std::min(src.length, destination.length) bytes from src to
 * destination
 *
 * \ingroup group_internal
 */
void block_copy(const block& source, block& destination) noexcept;

/**
 * Returns a upper rounded value of multiples of a
 * \ingroup group_internal
 */
inline constexpr size_t round_to_alignment(std::size_t basis, std::size_t n) noexcept {
    //      auto remainder = n % basis;
    //      return n + ((remainder == 0) ? 0 : (basis - remainder));
    return n + ((n % basis == 0) ? 0 : (basis - n % basis));
}

} // namespace internal
} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif