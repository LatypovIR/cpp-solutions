#pragma once

#include "weak_ptr.h"

#include "inplace_control_block.h"
#include "pointer_control_block.h"

template <typename T>
struct shared_ptr {
public:
  shared_ptr() noexcept : cb(nullptr), data(nullptr){};

  shared_ptr(std::nullptr_t) noexcept : shared_ptr(){};

  template <typename V>
  shared_ptr(shared_ptr<V> const& other, T* element) noexcept
      : cb(other.cb), data(element) {
    inc();
  };

  template <typename V, typename Deleter = std::default_delete<V>,
            std::enable_if_t<std::is_convertible_v<V&, T&>, bool> = true>
  shared_ptr(V* ptr, Deleter&& deleter = Deleter()) try
      : cb(new control_block::pointer_control_block(ptr, std::forward<Deleter>(deleter))),
        data(ptr) {
  } catch (...) {
    deleter(ptr);
    throw;
  };

  shared_ptr(shared_ptr const& other) noexcept
      : shared_ptr(other, other.data){};

  template <typename V,
            std::enable_if_t<std::is_convertible_v<V&, T&>, bool> = true>
  shared_ptr(shared_ptr<V> const& other) noexcept
      : shared_ptr(other, other.data){};

  shared_ptr(shared_ptr&& other) noexcept : cb(other.cb), data(other.data) {
    other.cb = nullptr;
    other.data = nullptr;
  };

  shared_ptr& operator=(const shared_ptr& other) noexcept {
    if (&other == this) {
      return *this;
    }

    dec();
    cb = other.cb;
    data = other.data;
    inc();

    return *this;
  };

  shared_ptr& operator=(shared_ptr&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    dec();
    cb = other.cb;
    data = other.data;
    other.cb = nullptr;
    other.data = nullptr;

    return *this;
  };

  T* get() const noexcept {
    return data;
  };

  operator bool() const noexcept {
    return data != nullptr;
  };

  T& operator*() const noexcept {
    return *get();
  };

  T* operator->() const noexcept {
    return get();
  };

  std::size_t use_count() const noexcept {
    return !cb ? 0 : cb->get_strong_ref_count();
  };

  void reset() noexcept {
    shared_ptr<T>().swap(*this);
  };

  template <typename V, typename Deleter = std::default_delete<V>,
            std::enable_if_t<std::is_convertible_v<V&, T&>, bool> = true>
  void reset(V* new_ptr, Deleter&& deleter = Deleter()) {
    shared_ptr<T>(new_ptr, std::forward<Deleter>(deleter)).swap(*this);
  };

  void swap(shared_ptr<T>& other) {
    std::swap(cb, other.cb);
    std::swap(data, other.data);
  }

  ~shared_ptr() {
    dec();
  }

private:
  template <typename V>
  friend struct shared_ptr;

  friend struct weak_ptr<T>;

  template <typename U, typename... Args>
  friend shared_ptr<U> make_shared(Args&&...);

  control_block::base_control_block* cb;
  T* data;

  explicit shared_ptr(control_block::base_control_block* cb, T* data) : cb(cb), data(data) {}
  void inc() {
    if (cb) {
      cb->inc_strong_count();
    }
  }
  void dec() {
    if (cb) {
      cb->dec_strong_count();
    }
  }
};

template <typename T, typename... Args>
shared_ptr<T> make_shared(Args&&... args) {
  auto* a = new control_block::inplace_control_block<T>(std::forward<Args>(args)...);
  return shared_ptr<T>(a, a->get_pointer());
};

template <typename T>
bool operator==(std::nullptr_t, shared_ptr<T> ptr) {
  return !ptr.get();
}

template <typename T>
bool operator!=(std::nullptr_t, shared_ptr<T> ptr) {
  return ptr.get();
}

template <typename T>
bool operator==(shared_ptr<T> ptr, std::nullptr_t) {
  return !ptr.get();
}

template <typename T>
bool operator!=(shared_ptr<T> ptr, std::nullptr_t) {
  return ptr.get();
}
