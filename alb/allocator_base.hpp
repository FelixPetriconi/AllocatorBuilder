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

#include "internal/traits.hpp"
#include "internal/array_creation_evaluator.hpp"
#include "stl_allocator_adapter.hpp"
#include <utility>
#include <type_traits>
#include <stddef.h>
#include <memory>

namespace alb {
  inline namespace v_100 {

    template< class T, class Allocator, class... Args>
    std::shared_ptr<T> make_shared(const Allocator& alloc, Args&&... args)
    {
      auto localAllocator = std_allocator_adapter<T, Allocator>(alloc);
      return std::allocate_shared<T>(localAllocator, std::forward<Args>(args)...);
    }

    /**
     * A general purpose deleter that implements the deleter interface used in 
     * std::unique_ptr 
     * Precondition: The Allocator must be an instance of an appix_allocator
     */
    template <typename Allocator>
    class deleter
    {
      Allocator *allocator_;
      size_t extend_;
    public:
      using allocator = Allocator;

      deleter() : allocator_(nullptr), extend_(1) {}

      explicit deleter(Allocator& a)
        : allocator_(a)
      {}

      void set_allocator(Allocator& a)
      {
        allocator_ = &a;
      }

      void set_extend(size_t e)
      {
        extend_ = e;
      }

      template <typename U>
      void operator()(U* p) {
        if (p) {
          // in case that p is an array of U, all objects must be destroyed
          for (auto i = 0u; i < extend_; ++i) { p[i].~U(); }
          // The C++ standard allows that the allocated memory position is different to the 
          // pointer of the first element. In this case 
          auto realMemoryLocation = (extend_ == 1 || std::is_trivially_destructible<U>::value)? p :
            (static_cast<void*>(reinterpret_cast<char*>(p) - helpers::array_offset()));

          block pseudoBlock(realMemoryLocation , 1 );
          auto size_prefix = allocator_->outer_to_prefix(pseudoBlock);
          block realBlock(realMemoryLocation, *size_prefix);
          allocator_->deallocate(realBlock);
        }
      }
    };

    template<class T, typename Allocator>
    struct _Unique_if
    {
      typedef std::unique_ptr<T, deleter<Allocator>> _Single_object;
    };

    template<class T, typename Allocator>
    struct _Unique_if < T[], Allocator > {
      typedef std::unique_ptr<T[], deleter<Allocator>> _Unknown_bound;
    };

    template<class T, size_t N, typename Allocator>
    struct _Unique_if < T[N], Allocator > {
      typedef void _Known_bound;
    };

    template<class T, class Allocator, class... Args>
    typename _Unique_if<T, Allocator>::_Single_object make_unique(Allocator& a, Args&&... args) {
      auto b = a.allocate(sizeof(T));
      if (b)
      {
        auto p = a.outer_to_prefix(b);
        *p = static_cast<typename Allocator::prefix>(b.length);
        auto result = std::unique_ptr<T, deleter<Allocator>>(new (b.ptr) T(std::forward<Args>(args)...));
        result.get_deleter().set_allocator(a);
        return result;
      }
      return std::unique_ptr<T, deleter<Allocator>>();
    }

    template<class T, class Allocator>
    typename _Unique_if<T, Allocator>::_Unknown_bound make_unique(Allocator& a, size_t n) {
      typedef typename std::remove_extent<T>::type U;

      auto b = a.allocate(sizeof(U) * n + helpers::array_offset());
      if (b)
      {
        auto p = a.outer_to_prefix(b);
        *p = static_cast<typename Allocator::prefix>(b.length);
        auto result = std::unique_ptr<U[], deleter<Allocator>>(new (b.ptr) U[n]);
        result.get_deleter().set_allocator(a);
        result.get_deleter().set_extend(n);
        return result;
      }
      return std::unique_ptr<T, deleter<Allocator>>();
    }

    template<class T, class Allocator, class... Args>
    typename _Unique_if<T, deleter<Allocator>>::_Known_bound make_unique(Allocator&, Args&&...) = delete;
 

    namespace internal {

      /**
       * Copies std::min(source.length, destination.length) bytes from source to
       * destination
       *
       * \ingroup group_internal
       */
      void block_copy(const block &source, block &destination) noexcept;

      /**
       * Returns a upper rounded value of multiples of a
       * \ingroup group_internal
       */
      inline constexpr size_t round_to_alignment(size_t basis, size_t n) noexcept
      {
        //      auto remainder = n % basis;
        //      return n + ((remainder == 0) ? 0 : (basis - remainder));
        return n + ((n % basis == 0) ? 0 : (basis - n % basis));
      }

    } // namespace Helper
  }
  using namespace v_100;
}
