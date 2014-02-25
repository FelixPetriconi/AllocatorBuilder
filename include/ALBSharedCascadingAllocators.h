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
#include <mutex>
#include <atomic>

namespace ALB
{
  /**
   * This implements a cascade of allocators. If the first allocator cannot 
   * fullfil the given request, then a next one is created and the requested is
   * passed to it.
   * This class is thread safe.
   * @tparam Allocator Of this type Allocators get created.
   * \ingroup group_allocators group_shared
   */
  template <typename Allocator>
  class SharedCascadingAllocators {
    typename typedef Allocator allocator;

    struct Node {
      Node() : next(nullptr) {}

      Allocator allocator;
      std::atomic<Node*> next;
    };

    std::atomic<Node*>  _root;

    Block allocateNoGrow(size_t n) {
      Block result;
      Node* p = _root;
      while (p) {
        result = p->allocator.allocate(n);
        if (result) {
          return result;
        }
        p = p->next;
      }
      return result;
    }

  public:
    static const bool supports_truncated_deallocation = 
      Allocator::supports_truncated_deallocation;
    
    SharedCascadingAllocators() : _root(nullptr) {}

    /**
     * Frees all allocated memory!
     */
    ~SharedCascadingAllocators() {
      while (_root.load() != nullptr) {
        Node* old = _root;
        Node* next = old->next.load();
        if (_root.compare_exchange_weak(old, next)) {
          delete old;
        }
      }
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
        Node* newNode = new Node;
        Node* oldNullValue = nullptr;
        if (!_root.compare_exchange_strong(oldNullValue, newNode)) {
          // In the meantime an other thread already created a root node
          delete newNode;
        }     

        return allocateNoGrow(n);
      }

      // a new node must be appended
      Node* newNode = new Node;
      Node* oldNullNode = nullptr;
      Node* p;
      do {
        p = _root;
        while (p->next.load() != nullptr) {
          p = p->next;
        }
      } while (!p->next.compare_exchange_strong(oldNullNode, newNode));
      
      return allocateNoGrow(n);
    }

    /**
     * Frees the given block and resets it
     */
    void deallocate(Block& b) {
      if (!b) {
        return;
      }    
      if (!owns(b)) {
        BOOST_ASSERT_MSG(false, "It is not wise to let me deallocate a foreign Block!");
        return;
      }

      Node* p = _root;
      while (p) {
        if (p->allocator.owns(b)) {
          p->allocator.deallocate(b);
          return;
        }
        p = p->next.load();
      }
    }

    /**
     * Reallocates the given block to the specified size.
     * If the owning allocator cannot fulfill the request then a cross move is performed
     * @param b Block to be reallocated
     * @param n The new size
     * @param True, if the operation was successful
     */
    bool reallocate(Block& b, size_t n)
    {
      if (Helper::Reallocator<SharedCascadingAllocators>::isHandledDefault(*this, b, n)){
        return true;
      }

      Node* p = _root;
      while (p) {
        if (p->allocator.owns(b)) {
          if (p->allocator.reallocate(b, n)) {
            return true;
          }

          return Helper::reallocateWithCopy(*this, *this, b, n);
        }
        p = p->next.load();
      }
      BOOST_ASSERT(0);
      return false;
    }

    /**
     * Tries to expand the given block insito by the specified number of bytes
     * This is only available if the Allocator implements it
     * @param b The block to be expanded
     * @param delta The amount of bytes
     * @return True, if the operation was successful
     */
    typename Traits::enable_result_to<bool, Traits::has_expand<Allocator>::value>::type 
      expand(Block& b, size_t delta) 
    {
      Node* p = _root;
      while (p) {
        if (p->allocator.owns(b)) {
          return p->allocator.expand(b, delta);
        }
        p = p->next.load();
      }
      BOOST_ASSERT(0);
      return false;
    }

    /**
     * Checks for the ownership of the given block
     * @param b The block to check
     * @return True, if one of the allocator owns it.
     */
    bool owns(const Block& b) const {
      Node* p = _root;
      while (p) {
        if (p->allocator.owns(b)) {
          return true;
        }
        p = p->next.load();
      }
      return false;
    }
  };
}
