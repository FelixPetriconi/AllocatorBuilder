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
#include <memory>
#include <atomic>

namespace alb {
  namespace internal {

    template <typename T> class atomic_shared_ptr {
      std::shared_ptr<T> data_;

    public:
      constexpr atomic_shared_ptr() noexcept = default;

      explicit atomic_shared_ptr(std::shared_ptr<T> p) noexcept
      {
        auto current = std::atomic_load(&data_);
        while (!std::atomic_compare_exchange_weak(&data_, &current, p))
          ;
      }

      atomic_shared_ptr &operator=(std::shared_ptr<T> p) noexcept
      {
        auto current = std::atomic_load(&data_);
        while (!std::atomic_compare_exchange_weak(&data_, &current, p))
          ;
        return *this;
      }

      bool is_lock_free() const noexcept
      {
        return std::atomic_is_lock_free();
      }

      void store(std::shared_ptr<T> p, std::memory_order order = memory_order_seq_cst) noexcept
      {
        std::atomic_store_explicit(&data_, p, order);
      }

      std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const noexcept
      {
        return std::atomic_load_explicit(&data_, order);
      }

      std::shared_ptr<T> exchange(std::shared_ptr<T> p,
                                  std::memory_order order = std::memory_order_seq_cst) noexcept
      {
        return std::atomic_exchange_explicit(&data_, p, order);
      }

      bool compare_exchange_strong(std::shared_ptr<T> &old_value, std::shared_ptr<T> new_value,
                                   std::memory_order order = std::memory_order_seq_cst) noexcept;

      bool compare_exchange_strong(std::shared_ptr<T> &old_value, std::shared_ptr<T> new_value,
                                   std::memory_order success_order,
                                   std::memory_order failure_order) noexcept;

      bool compare_exchange_weak(std::shared_ptr<T> &old_value, std::shared_ptr<T> new_value,
                                 std::memory_order order = std::memory_order_seq_cst) noexcept;

      bool compare_exchange_weak(std::shared_ptr<T> &old_value, std::shared_ptr<T> new_value,
                                 std::memory_order success_order,
                                 std::memory_order failure_order) noexcept;

      operator std::shared_ptr<T>() const noexcept
      {
        return load();
      }
    };
  }
}
