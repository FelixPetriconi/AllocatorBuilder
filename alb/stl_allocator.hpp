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
  class stl_allocator<void, Allocator> {
  public:
    using pointer = void *;
    using const_pointer = const void *;
    using value_type = void;
    template <class U> struct rebind {
      typedef stl_allocator<U, Allocator> other;
    };
  };

  template <typename T, class Allocator> 
  class stl_allocator {
    typename Allocator::value_type &allocator_;

  public:
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using value_type = T;

    template <typename U> struct rebind {
      typedef stl_allocator<U, Allocator> other;
    };

    stl_allocator()
      : allocator_(Allocator::instance())
    {
    }
    stl_allocator(const stl_allocator &)
      : allocator_(Allocator::instance())
    {
    }

    template <typename U>
    explicit stl_allocator(const stl_allocator<U, Allocator> &)
      : allocator_(Allocator::instance())
    {
    }

    ~stl_allocator()
    {
    }

    pointer address(reference r) const
    {
      return &r;
    };
    const_pointer address(const_reference r) const
    {
      return &r;
    };

    T *allocate(std::size_t n, const void * /*hint*/ = nullptr)
    {
      auto b = allocator_.allocate(n * sizeof(T));
      if (b) {
        auto p = allocator_.outer_to_prefix(b);
        p->length = static_cast<unsigned>(b.length);
        return static_cast<T *>(b.ptr);
      }
      throw std::bad_alloc();
    }

    void deallocate(T *ptr, std::size_t n)
    {
      block pseudoBlock(ptr, n);
      auto p = allocator_.outer_to_prefix(pseudoBlock);
      block realBlock(ptr, p->length);
      allocator_.deallocate(realBlock);
    }

    size_t max_size() const
    { // estimate maximum array size
      return ((size_t)(-1) / sizeof(T));
    }

    void construct(pointer ptr)
    {
      ::new (static_cast<void *>(ptr)) T;
    };

    template <class U> void construct(pointer ptr, const U &val)
    {
      ::new (static_cast<void *>(ptr)) T(val);
    };

    void construct(pointer ptr, const T &val)
    {
      ::new (static_cast<void *>(ptr)) T(val);
    };

    void destroy(pointer p)
    {
      p->T::~T();
    };
  };

  template <class Allocator, typename T1, typename T2>
  bool operator==(const stl_allocator<T1, Allocator> &, const stl_allocator<T2, Allocator> &)
  {
    return true;
  }

  template <class Allocator, typename T1, typename T2>
  bool operator!=(const stl_allocator<T1, Allocator> &, const stl_allocator<T2, Allocator> &)
  {
    return false;
  }
}
