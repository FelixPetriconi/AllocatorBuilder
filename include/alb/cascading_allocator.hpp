///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#ifndef ALB_CASCADING_ALLOCATOR_HPP
#define ALB_CASCADING_ALLOCATOR_HPP

#include <alb/allocator_base.hpp>
#include <alb/config.hpp>
#include <alb/internal/noatomic.hpp>
#include <alb/internal/reallocator.hpp>

#include <atomic>
#include <cassert>

namespace alb {
inline namespace ALB_VERSION_NAMESPACE() {
/**
 * This implements a cascade of allocators. If the first allocator_ cannot
 * fulfill the given request, then a next_ one is created and the requested is
 * passed to it.
 * This class is thread safe as far as not deleteAll is called.
 * \tparam Allocator of this type Allocators get created.
 *
 * \ingroup group_allocators group_shared
 */
template <bool Shared, typename Allocator>
class cascading_allocator_base {
    struct node;
    using node_ptr =
        typename traits::type_switch_t<std::atomic<node*>, internal::no_atomic<node*>, Shared>;

    struct node {
        node() noexcept : next_{}, allocated_this_size_{} {}

        node(node&& x) noexcept { *this = std::move(x); }

        node& operator=(node&& x) noexcept {
            allocator_ = std::move(x.allocator_);
            next_ = x.next_.load();
            allocated_this_size_ = x.allocated_this_size_;

            x.next_ = nullptr;
            x.allocated_this_size_ = 0;

            return *this;
        }

        Allocator allocator_;
        node_ptr next_;
        std::size_t allocated_this_size_;
    };

    node_ptr root_;

    block allocate_no_grow(std::size_t n) noexcept {
        block result;
        auto p = root_.load();
        while (p) {
            result = p->allocator_.allocate(n);
            if (result) {
                return result;
            }
            if (!p->next_.load()) {
                break;
            }
            p = p->next_.load();
        }
        return result;
    }

    node* create_node() noexcept {
        // Create a temporary node with an allocator_ on the stack
        node node_on_stack;

        // Use this allocator_ to create the first node in allocators space
        auto nodeBlock = node_on_stack.allocator_.allocate(sizeof(node));

        node_on_stack.allocated_this_size_ = nodeBlock.length;
        auto result = static_cast<node*>(nodeBlock.ptr);

        if (!result) {
            return nullptr;
        }

        // Create a new node emplace
        new (result) node();

        // Move the node from the stack to the allocated space
        *result = std::move(node_on_stack);

        return result;
    }

    /**
     * deletes the passed node and all decedents if available
     */
    void erase_node(node* n) noexcept {
        if (n == nullptr) {
            return;
        }
        if (n->next_.load()) {
            // delete all possible next_ Nodes in the list
            erase_node(n->next_.load());
            n->next_ = nullptr;
        }
        // Create a temporary node on the stack
        node stack_node;

        // Move the allocator_ to the temporary node
        stack_node = std::move(*n);
        block allocatedBlock(n, stack_node.allocated_this_size_);

        stack_node.allocator_.deallocate(allocatedBlock);
    }

    void shrink() noexcept { erase_node(root_.load()); }

    node* find_owning_node(const block& b) const noexcept {
        auto p = root_.load();
        while (p) {
            if (p->allocator_.owns(b)) {
                return p;
            }
            p = p->next_.load();
        }
        return nullptr;
    }

    cascading_allocator_base(const cascading_allocator_base&) = delete;
    cascading_allocator_base& operator=(const cascading_allocator_base&) = delete;

public:
    using allocator = Allocator;

    static constexpr bool supports_truncated_deallocation =
        Allocator::supports_truncated_deallocation;
    static constexpr unsigned alignment = Allocator::alignment;

    cascading_allocator_base() noexcept : root_(nullptr) {}

    static constexpr size_t good_size(size_t n) { return Allocator::good_size(n); }

    cascading_allocator_base(cascading_allocator_base&& x) noexcept { *this = std::move(x); }

    cascading_allocator_base& operator=(cascading_allocator_base&& x) noexcept {
        if (this == &x) {
            return *this;
        }
        shrink();
        root_ = std::move(x.root_);
        x.root_ = nullptr;
        return *this;
    }

    /**
     * Frees all allocated memory!
     */
    ~cascading_allocator_base() { shrink(); }

    /**
     * Sends the request to the first allocator_, if it cannot fulfill the request
     * then the next_ Allocator is created and so on
     */
    block allocate(size_t n) noexcept {
        if (n == 0) {
            return {};
        }

        block result = allocate_no_grow(n);
        if (result) {
            return result;
        }

        // no node at all there
        if (root_.load() == nullptr) {
            auto first_node = create_node();
            node* null_node = nullptr;
            // test if in the meantime someone else has created a node
            if (!root_.compare_exchange_weak(null_node, first_node)) {
                erase_node(first_node);
            }

            result = allocate_no_grow(n);
            if (result) {
                return result;
            }
        }

        // a new node must be appended
        auto new_node = create_node();
        node* null_node = nullptr;
        auto p = root_.load();
        do {
            p = root_;
            while (p->next_.load() != nullptr) {
                p = p->next_;
            }
        } while (!p->next_.compare_exchange_weak(null_node, new_node));

        result = allocate_no_grow(n);
        return result;
    }

    /**
     * Frees the given block and resets it
     */
    void deallocate(block& b) noexcept {
        if (!b) {
            return;
        }

        if (!owns(b)) {
            assert(!"It is not wise to let me deallocate a foreign Block!");
            return;
        }

        auto p = find_owning_node(b);
        if (p != nullptr) {
            p->allocator_.deallocate(b);
        }
    }

    /**
     * Reallocates the given block to the specified size.
     * If the owning allocator_ cannot fulfill the request then a cross move is
     * performed
     * \param b Block to be reallocated
     * \param n The new size
     * \param True, if the operation was successful
     */
    bool reallocate(block& b, size_t n) noexcept {
        if (internal::is_reallocation_handled_default(*this, b, n)) {
            return true;
        }

        auto p = find_owning_node(b);
        if (p == nullptr) {
            return false;
        }

        if (p->allocator_.reallocate(b, n)) {
            return true;
        }

        return internal::reallocate_with_copy(*this, *this, b, n);
    }

    /**
     * Tries to expand the given block insito by the specified number of bytes
     * This is only available if the Allocator implements it
     * \param b The block to be expanded
     * \param delta The amount of bytes
     * \return True, if the operation was successful
     */
    template <typename U = Allocator>
    typename std::enable_if_t<traits::has_expand_v<U>, bool> expand(block& b,
                                                                         size_t delta) noexcept {
        auto p = find_owning_node(b);
        if (p == nullptr) {
            return false;
        }
        return p->allocator_.expand(b, delta);
    }

    /**
     * Checks for the ownership of the given block
     * \param b The block to check
     * \return True, if one of the allocator_ owns it.
     */
    bool owns(const block& b) const noexcept { return find_owning_node(b) != nullptr; }

    /**
     * Deletes all allocated resources. All Blocks created by this instance
     * must not be used any more. Calling this method while other threads
     * are allocating or deallocating leads to unpredictable behavior.
     * This is only available if the Allocator implements it as well.
     */
    template <typename U = Allocator>
    typename std::enable_if_t<traits::has_deallocate_all_v<U>, void>
        deallocate_all() noexcept {
        shrink();
    }
};

/**
 * This class implements a thread safe cascading allocator_. For details see
 * ALB::CascadingAllocatorsBase
 * \tparam Allocator The allocator_ that shall be cascaded
 *
 * \group group_shared group_allocators
 */
template <class Allocator>
class shared_cascading_allocator : public cascading_allocator_base<true, Allocator> {
public:
    shared_cascading_allocator() noexcept {}
};

/**
 * This class implements a non thread safe cascading allocator_. For details see
 * ALB::CascadingAllocatorsBase
 * \tparam Allocator The allocator_ that shall be cascaded
 *
 * \group group_allocators
 */
template <class Allocator>
class cascading_allocator : public cascading_allocator_base<false, Allocator> {
public:
    cascading_allocator() noexcept = default;
};

} // namespace ALB_VERSION_NAMESPACE()
} // namespace alb

#endif