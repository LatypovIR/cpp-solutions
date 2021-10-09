#pragma once

#include "base_control_block.h"

template <typename T>
struct shared_ptr;

template <typename T>
struct weak_ptr {
public:
  weak_ptr() noexcept : cb(nullptr), data(nullptr) {};

  weak_ptr(const shared_ptr<T>& other) noexcept : cb(other.cb), data(other.data) {
    inc();
  };

  weak_ptr(const weak_ptr<T>& other) noexcept : cb(other.cb), data(other.data) {
    inc();
  };

  weak_ptr(weak_ptr<T>&& other) noexcept : cb(other.cb), data(other.data) {
    other.cb = nullptr;
    other.data = nullptr;
  };

  weak_ptr& operator=(const shared_ptr<T>& other) noexcept {
    dec();

    cb = other.cb;
    data = other.data;

    inc();
    return *this;
  };

  weak_ptr& operator=(const weak_ptr<T>& other) noexcept {
    if (this == &other) {
      return *this;
    }

    dec();

    cb = other.cb;
    data = other.data;

    inc();
    return *this;
  };


  weak_ptr& operator=(weak_ptr<T>&& other) noexcept {
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

  shared_ptr<T> lock() const noexcept {
    if (!cb || cb->get_strong_ref_count() == 0) {
      return shared_ptr<T>();
    }
    cb->inc_strong_count();
    return shared_ptr<T>(cb, data);
  };

  ~weak_ptr() {
    dec();
  }

private:
  control_block::base_control_block* cb;
  T* data;

  void inc() {
    if (cb) {
      cb->inc_weak_count();
    }
  }
  void dec() {
    if (cb) {
      cb->dec_weak_count();
    }
  }

};
