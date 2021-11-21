#pragma once

struct nullopt_t {};
nullopt_t nullopt;

struct in_place_t {};
in_place_t in_place;

#include <type_traits>
#include <memory>
#include <iostream>

struct dummy_t {};

template<typename T, bool IsTriviallyDestructible>
struct optional_destructor_base {

  constexpr optional_destructor_base() noexcept : _{} {};
  constexpr optional_destructor_base(T const& value) : value(value), has_value(true) {}
  constexpr optional_destructor_base(T&& value) : value(std::move(value)), has_value(true) {}

  template <typename... Args>
  constexpr optional_destructor_base(in_place_t, Args&&... args) :
        value(std::forward<Args>(args)...), has_value(true) {}

  ~optional_destructor_base() {
    if (has_value) {
      value.~T();
    }
  }

  union {
    T value;
    bool _{};
  };

  bool has_value = false;
};

template<typename T>
struct optional_destructor_base<T, true> {

  constexpr optional_destructor_base() noexcept : _{} {};
  constexpr optional_destructor_base(T const& value) : value(value), has_value(true) {}
  constexpr optional_destructor_base(T&& value) : value(std::move(value)), has_value(true) {}

  template <typename... Args>
  constexpr optional_destructor_base(in_place_t, Args&&... args) :
        value(std::forward<Args>(args)...), has_value(true) {}

  ~optional_destructor_base() = default;

  union {
    T value;
    bool _{};
  };

  bool has_value = false;
};

template <typename T>
using destructor_base = optional_destructor_base<T, std::is_trivially_destructible_v<T>>;

template<typename T, bool IsTriviallyCopyable>
struct optional_copyable_base : destructor_base<T> {
  using base = destructor_base<T>;
  using base::base;

  constexpr optional_copyable_base() noexcept = default;

  constexpr optional_copyable_base(optional_copyable_base const& other) : base() {
    *this = other;
  }

  constexpr optional_copyable_base(optional_copyable_base&& other) : base() {
    *this = std::move(other);
  }

  constexpr optional_copyable_base& operator=(optional_copyable_base const& other) {
    if (this->has_value) {
      this->value.~T();
    }
    this->has_value = other.has_value;
    if (other.has_value) {
      new (&this->value) T(other.value);
    }
    return *this;
  };

  constexpr optional_copyable_base& operator=(optional_copyable_base && other) {
    if (this->has_value) {
      this->value.~T();
    }
    this->has_value = other.has_value;
    if (other.has_value) {
      new (&this->value) T(std::move(other.value));
    }
    return *this;
  };
};

template<typename T>
struct optional_copyable_base<T, true> : destructor_base<T> {
  using base = destructor_base<T>;
  using base::base;

  constexpr optional_copyable_base() noexcept = default;
  constexpr optional_copyable_base(optional_copyable_base &&) = default;
  constexpr optional_copyable_base(optional_copyable_base const&) = default;
  constexpr optional_copyable_base& operator=(optional_copyable_base &&) = default;
  constexpr optional_copyable_base& operator=(optional_copyable_base const&) = default;
};

template <typename T>
using copyable_destructable_base = optional_copyable_base<T, std::is_trivially_copyable_v<T>>;

template<bool enable>
struct enable_copy {
  constexpr enable_copy() noexcept = default;
  constexpr enable_copy(enable_copy &&) = default;
  constexpr enable_copy(enable_copy const&) = delete;
  constexpr enable_copy& operator=(enable_copy &&) = default;
  constexpr enable_copy& operator=(enable_copy const&) = delete;
};

template<>
struct enable_copy<true> {
  constexpr enable_copy() noexcept = default;
  constexpr enable_copy(enable_copy &&) = default;
  constexpr enable_copy(enable_copy const&) = default;
  constexpr enable_copy& operator=(enable_copy &&) = default;
  constexpr enable_copy& operator=(enable_copy const&) = default;
};

template <typename T>
using enable_copy_base = enable_copy<std::is_copy_constructible_v<T>>;

template <typename T>
class optional : copyable_destructable_base<T>, enable_copy_base<T> {

  using base = copyable_destructable_base<T>;
  using base::base;

  public:
  constexpr optional() noexcept = default;
  constexpr optional(optional const&) = default;
  constexpr optional(optional&& other) = default;
  constexpr optional& operator=(optional&& other) = default;
  constexpr optional& operator=(optional const& other) = default;

  constexpr optional(nullopt_t) noexcept : optional() {};

  constexpr void reset() noexcept {
    if (this->has_value) {
      this->value.~T();
      this->has_value = false;
    }
  }

  constexpr optional& operator=(nullopt_t) noexcept {
    reset();
    return *this;
  };

  constexpr explicit operator bool() const noexcept {
      return this->has_value;
  };

  constexpr T& operator*() noexcept {
    return this->value;
  };

  constexpr T const& operator*() const noexcept {
    return this->value;
  };

  constexpr T* operator->() noexcept {
    return &operator*();
  };

  constexpr T const* operator->() const noexcept {
    return &operator*();
  };

  template <typename... Args>
  constexpr void emplace(Args&&... args) {
    reset();
    new (&this->value) T(std::forward<Args>(args)...);
    this->has_value = true;
  }
};

template <typename T>
constexpr bool operator==(optional<T> const& a, optional<T> const& b) {
  if (static_cast<bool>(a) != static_cast<bool>(b)) {
    return false;
  }
  if (!a && !b) {
    return true;
  }
  return *a == *b;
}

template <typename T>
constexpr bool operator!=(optional<T> const& a, optional<T> const& b) {
    return !(a == b);
}

template <typename T>
constexpr bool operator<(optional<T> const& a, optional<T> const& b) {
    if (!b) {
      return false;
    }
    if (!a) {
      return true;
    }
    return *a < *b;
}

template <typename T>
constexpr bool operator<=(optional<T> const& a, optional<T> const& b) {
  return !(b < a);
}

template <typename T>
constexpr bool operator>(optional<T> const& a, optional<T> const& b) {
  return b < a;
}

template <typename T>
constexpr bool operator>=(optional<T> const& a, optional<T> const& b) {
  return !(a < b);
}