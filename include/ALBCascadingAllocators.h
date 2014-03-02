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

#include "ALBAllocatorBase.h"
#include <ALBHelperNoAtomic.h>
#include <boost/assert.hpp>
#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
#include <boost/noncopyable.hpp>
#endif
#include <atomic>



namespace ALB {
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
class CascadingAllocatorsBase
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
  : boost::noncopyable
#endif
{
  struct Node;

  typedef typename Traits::type_switch<std::atomic<Node*>, 
                              Helper::NoAtomic<Node*>, 
                              Shared>::type NodePtr;

  struct Node {
    Node() 
      : next(nullptr)
      , allocatedThisSize(0) {}
    
    Node& operator=(Node&& x) {
      allocator           = std::move(x.allocator);
      next                = x.next.load();
      allocatedThisSize   = x.allocatedThisSize;

      x.next              = nullptr;
      x.allocatedThisSize = 0;
      
      return *this;
    }

    Allocator allocator;
    NodePtr  next;
    size_t allocatedThisSize;
  };

  NodePtr _root;



  Block allocateNoGrow(size_t n) {
    Block result;
    Node *p = _root.load();
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

  Node* createNode() {
    // Create a temporary node with an allocator on the stack
    Node nodeOnStack;
    
    // Use this allocator to create the first node in allocators space
    auto nodeBlock = nodeOnStack.allocator.allocate(sizeof(Node));

    nodeOnStack.allocatedThisSize = nodeBlock.length;
    Node* result = reinterpret_cast<Node*>(nodeBlock.ptr);
    
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
   * deletes the passed node and all decendents if available
   */
  void eraseNode(Node* n) {
    if (n == nullptr) {
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
    Block allocatedBlock(n, stackNode.allocatedThisSize);
      
    stackNode.allocator.deallocate(allocatedBlock);
  }

  void shrink() {
    eraseNode(_root.load());
  }

  Node* findOwningNode(const Block& b) const {
    Node* p = _root.load();
    while (p) {
      if (p->allocator.owns(b)) {
        return p;
      }
      p = p->next.load();
    }
    return nullptr;
  }

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
  CascadingAllocatorsBase(const CascadingAllocatorsBase&) = delete;
  const CascadingAllocatorsBase& operator=(const CascadingAllocatorsBase&) = delete;
#endif

public:
  typedef Allocator allocator;

  static const bool supports_truncated_deallocation =
      Allocator::supports_truncated_deallocation;

  CascadingAllocatorsBase() : _root(nullptr) {}

  CascadingAllocatorsBase(CascadingAllocatorsBase&& x) {
    *this = std::move(x);
  }

  CascadingAllocatorsBase& operator=(CascadingAllocatorsBase&& x) {
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
  ~CascadingAllocatorsBase() {
    shrink();
  }

  /**
   * Sends the request to the first allocator, if it cannot fulfill the request
   * then the next Allocator is created and so on
   */
  Block allocate(size_t n) {
    Block result = allocateNoGrow(n);
    if (result) {
      return result;
    }

    // no node at all there
    if (_root.load() == nullptr) {
      Node* firstNode = createNode();
      Node* nullNode = nullptr;
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
    Node *newNode = createNode();
    Node *nullNode = nullptr;
    Node *p;
    do {
      p = _root;
      while (p->next.load() != nullptr) {
        p = p->next;
      }
    } while (!p->next.compare_exchange_strong(nullNode, newNode));

    result = allocateNoGrow(n);
    return result;
  }

  /**
   * Frees the given block and resets it
   */
  void deallocate(Block &b) {
    if (!b) {
      return;
    }
    
    if (!owns(b)) {
      BOOST_ASSERT_MSG(false,
                       "It is not wise to let me deallocate a foreign Block!");
      return;
    }

    Node *p = findOwningNode(b);
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
  bool reallocate(Block &b, size_t n) {
    if (Helper::Reallocator<decltype(*this)>::isHandledDefault(
            *this, b, n)) {
      return true;
    }

    Node *p = findOwningNode(b);
    if (p == nullptr) {
      return false;
    }
    
    if (p->allocator.reallocate(b, n)) {
      return true;
    }

    return Helper::reallocateWithCopy(*this, *this, b, n);
  }

  /**
   * Tries to expand the given block insito by the specified number of bytes
   * This is only available if the Allocator implements it
   * \param b The block to be expanded
   * \param delta The amount of bytes
   * \return True, if the operation was successful
   */
  typename Traits::enable_result_to<bool,
                                    Traits::has_expand<Allocator>::value>::type
  expand(Block &b, size_t delta) {
    Node *p = findOwningNode(b);
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
  bool owns(const Block &b) const {
    return findOwningNode(b) != nullptr;
  }

  /**
   * Deletes all allocated resources. All Blocks created by this instance
   * must not be used any more. Calling this method while other threads
   * are allocating or deallocating leads to unpredictable behavior.
   * This is only available if the Allocator implements it as well.
   */
  typename Traits::enable_result_to<bool,
    Traits::has_deallocateAll<Allocator>::value>::type
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
class SharedCascadingAllocators : public CascadingAllocatorsBase<true, Allocator> {
public:
  SharedCascadingAllocators() {}
};


/**
 * This class implements a non thread safe cascading allocator. For details see
 * ALB::CascadingAllocatorsBase
 * \tparam Allocator The allocator that shall be cascaded
 *
 * \group group_allocators
 */
template <class Allocator>
class CascadingAllocators : public CascadingAllocatorsBase<false, Allocator> {
public:
  CascadingAllocators() {}
};

}
