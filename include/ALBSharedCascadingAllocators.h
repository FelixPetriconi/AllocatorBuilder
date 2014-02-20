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
    SharedCascadingAllocators() : _root(nullptr) {}

    ~SharedCascadingAllocators() {
      while (_root.load() != nullptr) {
        Node* old = _root;
        Node* next = old->next.load();
        if (_root.compare_exchange_weak(old, next)) {
          delete old;
        }
      }
    }

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

    void deallocate(Block& b) {
      if (!b) {
        return;
      }    
      BOOST_ASSERT_MSG(owns(b), "It is not wise to let me deallocate a foreign Block!");
      if (!owns(b)) {
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
     * This method is only available when Allocator implements ::expand
     */
    typename Traits::enabled<Traits::has_expand<Allocator>::value>::type 
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
