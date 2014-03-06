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

#include <cstddef>
#include "allocator_base.hpp"

namespace alb {

template <typename T, class Allocator>
class stl_allocator;

template <class Allocator> 
class stl_allocator<void, Allocator>
{
public:
  typedef void * pointer;
  typedef const void* const_pointer;
  typedef void value_type;
  template <class U> 
  struct rebind {
    typedef stl_allocator<U, Allocator> other;
  };
};


template <typename T, class Allocator>
class stl_allocator {
  typename Allocator::value_type& _allocator;

public:
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  typedef T value_type;
  
  template <typename U>
  struct rebind {
    typedef stl_allocator<U, Allocator> other;
  };

  stl_allocator() : _allocator(Allocator::instance()) {}
  stl_allocator(const stl_allocator&) : _allocator(Allocator::instance()) {}

  template <typename U>
  stl_allocator(const stl_allocator<U, Allocator>&) : _allocator(Allocator::instance()) {}

  ~stl_allocator() {}

  pointer address(reference r) const { return &r; };
  const_pointer address(const_reference r) const { return &r; };

  T* allocate(std::size_t n, const void* /*hint*/ = nullptr) {
    auto b = _allocator.allocate(n * sizeof(T));
    if (b) {
      auto p = _allocator.outerToPrefix(b);
      p->length = static_cast<unsigned>(b.length);
      return static_cast<T*>(b.ptr);
    } 
    throw std::bad_alloc();
  }
  
  void deallocate(T* ptr, std::size_t n) {
  block pseudoBlock(ptr, n);
    auto p = _allocator.outerToPrefix(pseudoBlock);
    block realBlock(ptr, p->length);
    _allocator.deallocate(realBlock);
  }

  size_t max_size() const
  {	// estimate maximum array size
    return ((size_t)(-1) / sizeof (T));
  }

  void construct(pointer ptr)
  {
    ::new (reinterpret_cast<void*>(ptr)) T;
  };

  template <class U> 
  void construct(pointer ptr, const U& val)
  {
    ::new (reinterpret_cast<void*>(ptr)) T(val);
  };

  void construct(pointer ptr, const T& val)
  {
    ::new (reinterpret_cast<void*>(ptr)) T(val);
  };

  void destroy(pointer p)
  {
    p->T::~T();
  };
};

template <class Allocator, typename T1, typename T2>
bool operator==(const stl_allocator<T1, Allocator>&, const stl_allocator<T2, Allocator>&) {
  return true;
}

template <class Allocator, typename T1, typename T2>
bool operator!=(const stl_allocator<T1, Allocator>&, const stl_allocator<T2, Allocator>&) {
  return false;
}

}
