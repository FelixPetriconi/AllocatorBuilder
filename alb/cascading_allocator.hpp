///////////////////////////////////////////////////////////////////
//
// Copyright 2014 Felix Petriconi
//
// License: http://boost.org/LICENSE_1_0.txt, Boost License 1.0
//
// Authors: http://petriconi.net, Felix Petriconi
//
///////////////////////////////////////////////////////////////////
#pragma once

#include "allocator_base.hpp"
#include "internal/noatomic.hpp"
#include "internal/reallocator.hpp"
#include <atomic>
#include <cassert>

namespace alb {

  /**
   * This implements a cascade of allocators. If the first allocator cannot
   * fulfill the given request, then a next one is created and the requested is
   * passed to it.
   * This class is thread safe as far as not deleteAll is called.
   * \tparam Allocator of this type Allocators get created.
   *
   * \ingroup group_allocators group_shared
   */
  template <bool Shared, typename Allocator> class cascading_allocator_base {
    struct Node;
    using NodePtr =
        typename traits::type_switch<std::atomic<Node *>, internal::NoAtomic<Node *>, Shared>::type;

    struct Node {
      Node() noexcept
        : next{nullptr}
        , allocatedThisSize{0}
      {
      }

      Node(Node &&x) noexcept
      {
        *this = std::move(x);
      }

      Node &operator=(Node &&x) noexcept
      {
        allocator = std::move(x.allocator);
        next = x.next.load();
        allocatedThisSize = x.allocatedThisSize;

        x.next = nullptr;
        x.allocatedThisSize = 0;

        return *this;
      }

      Allocator allocator;
      NodePtr next;
      size_t allocatedThisSize;
    };

    NodePtr root_;

    block allocate_no_grow(size_t n) noexcept
    {
      block result;
      auto p = root_.load();
      while (p) {
        result = p->allocator.allocate(n);
        if (result) {
          return result;
        }
        if (!p->next.load()) {
          break;
        }
        p = p->next.load();
      }
      return result;
    }

    Node *create_node() noexcept
    {
      // Create a temporary node with an allocator on the stack
      Node nodeOnStack;

      // Use this allocator to create the first node in allocators space
      auto nodeBlock = nodeOnStack.allocator.allocate(sizeof(Node));

      nodeOnStack.allocatedThisSize = nodeBlock.length;
      auto result = static_cast<Node *>(nodeBlock.ptr);

      if (!result) {
        return nullptr;
      }

      // Create a new Node emplace
      new (result) Node();

      // Move the node from the stack to the allocated space
      *result = std::move(nodeOnStack);

      return result;
    }

    /**
     * deletes the passed node and all decedents if available
     */
    void erase_node(Node *n) noexcept
    {
      if (n == nullptr) {
        return;
      }
      if (n->next.load()) {
        // delete all possible next Nodes in the list
        erase_node(n->next.load());
        n->next = nullptr;
      }
      // Create a temporary node on the stack
      Node stackNode;

      // Move the allocator to the temporary node
      stackNode = std::move(*n);
      block allocatedBlock(n, stackNode.allocatedThisSize);

      stackNode.allocator.deallocate(allocatedBlock);
    }

    void shrink() noexcept
    {
      erase_node(root_.load());
    }

    Node *find_owning_node(const block &b) const noexcept
    {
      auto p = root_.load();
      while (p) {
        if (p->allocator.owns(b)) {
          return p;
        }
        p = p->next.load();
      }
      return nullptr;
    }

    cascading_allocator_base(const cascading_allocator_base &) = delete;
    cascading_allocator_base &operator=(const cascading_allocator_base &) = delete;

  public:
    using allocator = Allocator;

    static constexpr bool supports_truncated_deallocation = Allocator::supports_truncated_deallocation;

    cascading_allocator_base() noexcept
      : root_(nullptr)
    {
    }

    static constexpr size_t good_size(size_t n) {
      return Allocator::good_size(n);
    }

    cascading_allocator_base(cascading_allocator_base &&x) noexcept
    {
      *this = std::move(x);
    }

    cascading_allocator_base &operator=(cascading_allocator_base &&x) noexcept
    {
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
    ~cascading_allocator_base()
    {
      shrink();
    }

    /**
     * Sends the request to the first allocator, if it cannot fulfill the request
     * then the next Allocator is created and so on
     */
    block allocate(size_t n) noexcept
    {
      if (n == 0) {
        return {};
      }

      block result = allocate_no_grow(n);
      if (result) {
        return result;
      }

      // no node at all there
      if (root_.load() == nullptr) {
        auto firstNode = create_node();
        Node *nullNode = nullptr;
        // test if in the meantime someone else has created a node
        if (!root_.compare_exchange_weak(nullNode, firstNode)) {
          erase_node(firstNode);
        }

        result = allocate_no_grow(n);
        if (result) {
          return result;
        }
      }

      // a new node must be appended
      auto newNode = create_node();
      Node *nullNode = nullptr;
      auto p = root_.load();
      do {
        p = root_;
        while (p->next.load() != nullptr) {
          p = p->next;
        }
      } while (!p->next.compare_exchange_weak(nullNode, newNode));

      result = allocate_no_grow(n);
      return result;
    }

    /**
     * Frees the given block and resets it
     */
    void deallocate(block &b) noexcept
    {
      if (!b) {
        return;
      }

      if (!owns(b)) {
        assert(!"It is not wise to let me deallocate a foreign Block!");
        return;
      }

      auto p = find_owning_node(b);
      if (p != nullptr) {
        p->allocator.deallocate(b);
      }
    }

    /**
     * Reallocates the given block to the specified size.
     * If the owning allocator cannot fulfill the request then a cross move is
     * performed
     * \param b Block to be reallocated
     * \param n The new size
     * \param True, if the operation was successful
     */
    bool reallocate(block &b, size_t n) noexcept
    {
      if (internal::reallocator<decltype(*this)>::is_handled_default(*this, b, n)) {
        return true;
      }

      auto p = find_owning_node(b);
      if (p == nullptr) {
        return false;
      }

      if (p->allocator.reallocate(b, n)) {
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
    template<typename U = Allocator>
    typename std::enable_if<traits::has_expand<U>::value, bool>::type
    expand(block &b, size_t delta) noexcept
    {
      auto p = find_owning_node(b);
      if (p == nullptr) {
        return false;
      }
      return p->allocator.expand(b, delta);
    }

    /**
     * Checks for the ownership of the given block
     * \param b The block to check
     * \return True, if one of the allocator owns it.
     */
    bool owns(const block &b) const noexcept
    {
      return find_owning_node(b) != nullptr;
    }

    /**
     * Deletes all allocated resources. All Blocks created by this instance
     * must not be used any more. Calling this method while other threads
     * are allocating or deallocating leads to unpredictable behavior.
     * This is only available if the Allocator implements it as well.
     */
    template <typename U = Allocator>
    typename std::enable_if<traits::has_deallocate_all<U>::value, void>::type
    deallocate_all() noexcept
    {
      shrink();
    }
  };

  /**
   * This class implements a thread safe cascading allocator. For details see
   * ALB::CascadingAllocatorsBase
   * \tparam Allocator The allocator that shall be cascaded
   *
   * \group group_shared group_allocators
   */
  template <class Allocator>
  class shared_cascading_allocator : public cascading_allocator_base<true, Allocator> {
  public:
    shared_cascading_allocator() noexcept
    {
    }
  };

  /**
   * This class implements a non thread safe cascading allocator. For details see
   * ALB::CascadingAllocatorsBase
   * \tparam Allocator The allocator that shall be cascaded
   *
   * \group group_allocators
   */
  template <class Allocator>
  class cascading_allocator : public cascading_allocator_base<false, Allocator> {
  public:
    cascading_allocator() noexcept
    {
    }
  };
}
