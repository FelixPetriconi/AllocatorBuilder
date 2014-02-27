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
#include <boost/assert.hpp>
#include <boost/config.hpp>
#include <mutex>

namespace ALB {
/**
 * This implements a cascade of allocators. If the first allocator cannot
 * fulfill the given request, then a next one is created and the requested is
 * passed to it.
 * This class is thread safe.
 * \tparam Allocator of this type Allocators get created.
 *
 * \ingroup group_allocators group_shared
 */
template <typename Allocator> 
class SharedCascadingAllocators 
#ifdef BOOST_NO_CXX11_DELETED_FUNCTIONS
  : boost::noncopyable
#endif
{
  struct Node {
    Node() 
      : next(nullptr)
      , allocatedThisSize(0)
      , nextIsInitialized(false) {}
    
    Node& operator=(Node&& x) {
      allocator           = std::move(x.allocator);
      next                = std::move(x.next);
      allocatedThisSize   = x.allocatedThisSize;
      nextIsInitialized   = x.nextIsInitialized;

      x.next              = nullptr;
      x.allocatedThisSize = 0;
      x.nextIsInitialized = false;
      
      return *this;
    }

    Allocator allocator;
    Node    *next;
    size_t allocatedThisSize;
    bool nextIsInitialized;
  };

  Node* _root;
  mutable std::recursive_mutex _mutex;


  Block allocateNoGrow(size_t n) {
    std::lock_guard<std::recursive_mutex> guard(_mutex);

    Block result;
    Node *p = _root;
    while (p) {
      result = p->allocator.allocate(n);
      if (result) {
        return result;
      }
      if (!p->nextIsInitialized) {
        break;
      }
      p = p->next;
    }
    return result;
  }

  void eraseNode(Node* n) {
    if (n == nullptr) {
      return;
    }
    if (n->next) {
      if (n->nextIsInitialized) {
        eraseNode(n->next);
        Block allocatedBlock(n->next, n->next->allocatedThisSize);
        n->allocator.deallocate(allocatedBlock);
      }
      else {
        Block allocatedBlock(n->next, *(reinterpret_cast<size_t*>(n->next)));
        n->allocator.deallocate(allocatedBlock);
      }
    }
  }

  void shrink() {
    eraseNode(_root);
    // Finally we have to clear the very first node. Here we use the same trick
    // as during allocation, we move the allocator to the stack and from there
    // we delete the first node
    if (_root != nullptr) {
      Node stackNode;
      stackNode = std::move(*_root);
      Block rootBlock(_root, stackNode.allocatedThisSize);
      stackNode.allocator.deallocate(rootBlock);
      _root = nullptr;
    }
  }

  Node* findOwningNode(const Block& b) const {
    Node *p = _root;
    while (p) {
      if (p->allocator.owns(b)) {
        return p;
      }
      p = p->next;
    }
    return nullptr;
  }

#ifndef BOOST_NO_CXX11_DELETED_FUNCTIONS
  SharedCascadingAllocators(const SharedCascadingAllocators&) = delete;
  const SharedCascadingAllocators& operator=(const SharedCascadingAllocators&) = delete;
#endif

public:
  typename typedef Allocator allocator;

  static const bool supports_truncated_deallocation =
      Allocator::supports_truncated_deallocation;

  SharedCascadingAllocators() : _root(nullptr) {}

  SharedCascadingAllocators(SharedCascadingAllocators&& x) {
    *this = std::move(x);
  }

  SharedCascadingAllocators& operator=(SharedCascadingAllocators&& x) {
    if (this == &x) {
      return *this;
    }
    std::lock_guard<std::recursive_mutex> guardThis(_mutex);
    shrink();
    std::lock_guard<std::recursive_mutex> guardX(_mutex);
    _root = std::move(x._root);
    x._root = nullptr;
    return *this;
  }

  /**
   * Frees all allocated memory!
   */
  ~SharedCascadingAllocators() {
    std::lock_guard<std::recursive_mutex> guard(_mutex);
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
    if (_root == nullptr) {
      {
        std::lock_guard<std::recursive_mutex> guard(_mutex);
  
        // Create a temporary node with an allocator on the stack
        Node newNode = Node();
        // Use this allocator to create the first node in allocators space
        auto firstNodeBlock = newNode.allocator.allocate(sizeof(Node));
        newNode.allocatedThisSize = firstNodeBlock.length;
        Node* firstRoot = reinterpret_cast<Node*>(firstNodeBlock.ptr);
        new (firstRoot) Node();
  
        // If this allocations fails, then the is no room left and we have to quit
        if (!firstRoot) {
          return Block();
        }
        // Move the node from the stack to the allocated space
        *firstRoot = std::move(newNode);
        auto nextBlock = firstRoot->allocator.allocate(sizeof(Node));
        BOOST_ASSERT(nextBlock);
        firstRoot->next = reinterpret_cast<Node*>(nextBlock.ptr);
        // store temporarily the size of the node at the beginning of the allocated
        // space
        *(reinterpret_cast<size_t*>(firstRoot->next)) = nextBlock.length;
        
        _root = firstRoot;
      }
 
      return allocateNoGrow(n);
    }

    
    // a new node must be appended
    {
      std::lock_guard<std::recursive_mutex> guard(_mutex);
      auto currentNode = _root;
      while (currentNode->nextIsInitialized) {
        currentNode = currentNode->next;
      }
  
      // There is no next one 
      if (!currentNode->next) {
        return Block();
      }
  
      const size_t allocatedSizeOfNextNode = *(reinterpret_cast<size_t*>(currentNode->next));
      
      new (currentNode->next) Node();
      
      currentNode->allocatedThisSize = allocatedSizeOfNextNode;
      currentNode->nextIsInitialized = true;
  
      // reserve space for the next allocator
      auto nextBlock = currentNode->next->allocator.allocate(sizeof(Node));
      if (nextBlock) {
        currentNode->next->next = reinterpret_cast<Node*>(nextBlock.ptr);
        // store temporarily the size of the node at the beginning of the allocated
        // space
        *(reinterpret_cast<size_t*>(currentNode->next->next)) = nextBlock.length;
      }
    }

    return allocateNoGrow(n);
  }

  /**
   * Frees the given block and resets it
   */
  void deallocate(Block &b) {
    if (!b) {
      return;
    }
    
    std::lock_guard<std::recursive_mutex> guard(_mutex);
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
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    if (Helper::Reallocator<SharedCascadingAllocators>::isHandledDefault(
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
    std::lock_guard<std::recursive_mutex> guard(_mutex);
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
    std::lock_guard<std::recursive_mutex> guard(_mutex);
    return findOwningNode(b) != nullptr;
  }

  typename Traits::enable_result_to<bool,
    Traits::has_deallocateAll<Allocator>::value>::type
    deallocateAll() {
      boost::lock_guard<boost::shared_mutex> guard(_mutex);
    shrink();
  }
};
}
