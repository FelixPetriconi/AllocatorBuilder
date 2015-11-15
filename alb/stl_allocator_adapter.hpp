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
#include <utility>
#include "allocator_base.hpp"

namespace alb {

  inline namespace v_100 {

    template <typename T, class Allocator>
    class std_allocator_adapter;

    template <class Allocator>
    class std_allocator_adapter<void, Allocator> {
    public:
      using pointer = void *;
      using const_pointer = const void *;
      using value_type = void;
      template <class U> struct rebind {
        typedef std_allocator_adapter<U, Allocator> other;
      };
    };

    template <typename T, class Allocator>
    class std_allocator_adapter {
      const Allocator& allocator_;

    public:
      using size_type = size_t;
      using difference_type = ptrdiff_t;
      using pointer = T *;
      using const_pointer = const T *;
      using reference = T &;
      using const_reference = const T &;
      using value_type = T;

      template <typename U> struct rebind {
        typedef std_allocator_adapter<U, Allocator> other;
      };

      explicit std_allocator_adapter(const Allocator& allocator) noexcept
        : allocator_(allocator)
      {
      }

      ~std_allocator_adapter() noexcept {}

      const Allocator& allocator() const
      {
        return allocator_;
      }

      template <typename U>
      explicit std_allocator_adapter(const std_allocator_adapter<U, Allocator> &other)
        : allocator_(other.allocator())
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
        auto b = const_cast<Allocator&>(allocator_).allocate(n * sizeof(T));
        if (b) {
          auto p = allocator_.outer_to_prefix(b);
          *p = b.length;
          return static_cast<T *>(b.ptr);
        }
        return nullptr;
      }

      void deallocate(T *ptr, std::size_t n)
      {
        block pseudoBlock(ptr, n);
        auto p = allocator_.outer_to_prefix(pseudoBlock);
        block realBlock(ptr, *p);
        const_cast<Allocator&>(allocator_).deallocate(realBlock);
      }

      size_t max_size() const
      { // estimate maximum array size
        return ((size_t)(-1) / sizeof(T));
      }

      void construct(pointer ptr)
      {
        ::new (static_cast<void *>(ptr)) T;
      };

      template <class... U> 
      void construct(pointer ptr, U&&... val)
      {
        ::new (static_cast<void *>(ptr)) T(std::forward<U>(val)...);
      };

      void destroy(pointer p)
      {
        p->T::~T();
      };
    };

    template <class Allocator, typename T1, typename T2>
    bool operator==(const std_allocator_adapter<T1, Allocator> &, const std_allocator_adapter<T2, Allocator> &)
    {
      return true;
    }

    template <class Allocator, typename T1, typename T2>
    bool operator!=(const std_allocator_adapter<T1, Allocator> &x, const std_allocator_adapter<T2, Allocator> &y)
    {
      return !(x == y);
    }
  }

  using namespace v_100;
}
