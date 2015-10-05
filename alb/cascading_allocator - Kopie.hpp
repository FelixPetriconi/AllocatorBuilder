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
#include "internal/atomic_shared_ptr.h"

#include <boost/assert.hpp>

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
template <bool Shared, typename Allocator> 
class cascading_allocator_base
{
  struct Node;

  using NodePtr = typename traits::type_switch<internal::atomic_shared_ptr<Node>,
                              internal::NoAtomic<Node*>, 
                              Shared>::type;

  struct Node {
    Node() 
      : next(nullptr)
      , allocatedThisSize(0) {}
    
    Node(Node&& x) { *this = std::move(x); }

    Node& operator=(Node&& x) {
      allocator           = std::move(x.allocator);
      next                = x.next.load();
      allocatedThisSize   = x.allocatedThisSize;

      x.next              = nullptr;
      x.allocatedThisSize = 0;
      
      return *this;
    }
    
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    Allocator allocator;
    NodePtr  next;
    size_t allocatedThisSize;
  };

  NodePtr _root;



  block allocateNoGrow(size_t n) {
    block result;
    auto p = _root.load();
    while (p) {
      result = p->allocator.allocate(n);
      if (result) {
        return result;
      }
      if (!p->next) {
        break;
      }
      p = p->next;
    }
    return result;
  }

  template <typename N>
  N createNode();

  template <>
  std::shared_ptr<Node> createNode() {
    // Create a temporary node with an allocator on the stack
    Node nodeOnStack;
    
    // Use this allocator to create the first node in allocators space
    auto nodeBlock = nodeOnStack.allocator.allocate(sizeof(NodePtr));

    nodeOnStack.allocatedThisSize = nodeBlock.length;
    auto result = reinterpret_cast<NodeAllocationType*>(nodeBlock.ptr);
    
    if (!result) {
      return nullptr;
    }

    // Create a new Node emplace
    new (result) NodePtr();

    // Move the node from the stack to the allocated space
    result->reset(new Node(std::move(nodeOnStack)) );

    return NodePtr(result);
  }

  template <>
  Node* createNode() {
    // Create a temporary node with an allocator on the stack
    Node nodeOnStack;

    // Use this allocator to create the first node in allocators space
    auto nodeBlock = nodeOnStack.allocator.allocate(sizeof(Node));

    nodeOnStack.allocatedThisSize = nodeBlock.length;
    auto result = reinterpret_cast<Node*>(nodeBlock.ptr);

    if (!result) {
      return nullptr;
    }

    // Create a new Node emplace
    new (result) Node();

    // Move the node from the stack to the allocated space
    *result = std::move(nodeOnStack);

    return NodePtr(result);
  }

  /**
   * deletes the passed node and all decedents if available
   */
  template <typename N>
  void eraseNode(N);

  template <>
  void eraseNode(internal::atomic_shared_ptr<Node>& n) {
    if (!n) {
      return;
    }
    if (n->next) {
      // delete all possible next Nodes in the list
      eraseNode<NodePtr>(n->next);
      n->next = nullptr;
    }
    // Create a temporary node on the stack
    Node stackNode;

    // Move the allocator to the temporary node
    stackNode = std::move(*n);
    block allocatedBlock(n, stackNode.allocatedThisSize);
      
    stackNode.allocator.deallocate(allocatedBlock);
  }

  template <>
  void eraseNode(Node *n) {
    if (!n) {
      return;
    }
    if (n->next.load()) {
      // delete all possible next Nodes in the list
      eraseNode(n->next.load());
      n->next = nullptr;
    }
    // Create a temporary node on the stack
    Node stackNode;

    // Move the allocator to the temporary node
    stackNode = std::move(*n);
    block allocatedBlock(n, stackNode.allocatedThisSize);

    stackNode.allocator.deallocate(allocatedBlock);
  }

  void shrink() {
    eraseNode(_root);
  }

  NodePtr findOwningNode(const block& b) const {
    auto p = _root.load();
    while (p) {
      if (p->allocator.owns(b)) {
        return p;
      }
      p = p->next;
    }
    return NodePtr{nullptr};
  }

  cascading_allocator_base(const cascading_allocator_base&) = delete;
  cascading_allocator_base& operator=(const cascading_allocator_base&) = delete;

public:
  using allocator = Allocator;
  static const bool supports_truncated_deallocation =
      Allocator::supports_truncated_deallocation;

  cascading_allocator_base() : _root(nullptr) {}

  cascading_allocator_base(cascading_allocator_base&& x) {
    *this = std::move(x);
  }

  cascading_allocator_base& operator=(cascading_allocator_base&& x) {
    if (this == &x) {
      return *this;
    }
    shrink();
    _root = std::move(x._root);
    x._root = nullptr;
    return *this;
  }

  /**
   * Frees all allocated memory!
   */
  ~cascading_allocator_base() {
    shrink();
  }

  /**
   * Sends the request to the first allocator, if it cannot fulfill the request
   * then the next Allocator is created and so on
   */
  block allocate(size_t n) {
    auto result = allocateNoGrow(n);
    if (result) {
      return result;
    }

    // no node at all there
    auto r = _root.load();
    if (r == nullptr) {
      auto firstNode = createNode<NodePtr>();
      auto nullNode = NodePtr(nullptr);
      // test if in the meantime someone else has created a node
      if (!_root.compare_exchange_strong(nullNode, firstNode)) {
        eraseNode(firstNode);
      }

      result = allocateNoGrow(n);
      if (result) {
        return result;
      }
    }
    
    // a new node must be appended
    auto newNode = createNode<NodePtr>();
    auto nullNode = NodePtr(nullptr);
    auto p = _root.load();
    do {
      p = _root.load();
      while (p->next != nullptr) {
        p = p->next;
      }
    } while (!p->next.compare_exchange_strong(nullNode, newNode));

    result = allocateNoGrow(n);
    return result;
  }

  /**
   * Frees the given block and resets it
   */
  void deallocate(block& b) {
    if (!b) {
      return;
    }
    
    if (!owns(b)) {
      BOOST_ASSERT_MSG(false,
                       "It is not wise to let me deallocate a foreign Block!");
      return;
    }

    auto p = findOwningNode(b).load();
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
  bool reallocate(block& b, size_t n) {
    if (internal::reallocator<decltype(*this)>::isHandledDefault(
            *this, b, n)) {
      return true;
    }

    auto p = findOwningNode(b);
    if (p == nullptr) {
      return false;
    }
    
    if (p->allocator.reallocate(b, n)) {
      return true;
    }

    return internal::reallocateWithCopy(*this, *this, b, n);
  }

  /**
   * Tries to expand the given block insito by the specified number of bytes
   * This is only available if the Allocator implements it
   * \param b The block to be expanded
   * \param delta The amount of bytes
   * \return True, if the operation was successful
   */
  typename traits::enable_result_to<bool,
                                    traits::has_expand<Allocator>::value>::type
  expand(block& b, size_t delta) {
    auto p = findOwningNode(b);
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
  bool owns(const block& b) const {
    return findOwningNode(b) != nullptr;
  }

  /**
   * Deletes all allocated resources. All Blocks created by this instance
   * must not be used any more. Calling this method while other threads
   * are allocating or deallocating leads to unpredictable behavior.
   * This is only available if the Allocator implements it as well.
   */
  typename traits::enable_result_to<bool,
    traits::has_deallocateAll<Allocator>::value>::type
    deallocateAll() {
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
  shared_cascading_allocator() {}
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
  cascading_allocator() {}
};

}
