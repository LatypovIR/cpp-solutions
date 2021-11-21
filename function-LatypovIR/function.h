#pragma once

#include "function_structs.h"

template <typename F>
struct function;

template <typename R, typename... Args>
struct function<R (Args...)> {

  function() noexcept {
    data.ops = function_structs::get_empty_type_descriptor<R, Args...>();
  };

  function(function const& other) {
    other.data.copy(data);
  };

  function(function&& other) noexcept {
    other.data.move(data);
  };

  template <typename T>
  function(T func) {
    traits<T>::init(data, std::move(func));
  }

  function& operator=(function const& rhs) {
    if (this == &rhs) {
      return *this;
    }
    operator=(function(rhs));
    return *this;
  };

  function& operator=(function&& rhs) noexcept {
    if (this == &rhs) {
      return *this;
    }
    data.destroy();
    rhs.data.move(data);
    return *this;
  };

  ~function() {
    data.destroy();
  };

  explicit operator bool() const noexcept {
    return data.ops !=
           function_structs::get_empty_type_descriptor<R, Args...>();
  };

  R operator()(Args... args) const {
    return data.invoke(std::forward<Args>(args)...);
  };

  template <typename T>
  T* target() noexcept {
    if (data.ops == traits<T>::template get_object_descriptor<R, Args...>()) {
      return traits<T>::as_target(data);
    } else {
      return nullptr;
    }
  }

  template <typename T>
  T const* target() const noexcept {
    if (data.ops == traits<T>::template get_object_descriptor<R, Args...>()) {
      return traits<T>::as_target(data);
    } else {
      return nullptr;
    }
  }

private:
  function_structs::storage<R, Args...> data;

  template <typename T>
  using traits = function_structs::object_traits<T>;
};
