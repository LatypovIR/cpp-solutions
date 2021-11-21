#pragma once

#include <type_traits>

using bad_function_call = std::bad_function_call;

namespace function_structs {

template <typename T>
static constexpr bool is_small =
    sizeof(T) <= sizeof(T*) &&
    alignof(T*) % alignof(T) == 0 && std::is_nothrow_move_constructible_v<T>;

template <typename R, typename... Args>
struct type_descriptor;

template <typename R, typename... Args>
struct storage {

  template <typename T>
  T& get_reference_func() {
    return reinterpret_cast<T&>(buffer);
  }

  template <typename T>
  T const& get_reference_func() const {
    return reinterpret_cast<T const&>(buffer);
  }

  template <typename T>
  T* get_pointer_func() const {
    return reinterpret_cast<T* const&>(buffer);
  }

  void set_function(void* func) {
    new (&buffer) void*(func);
  }

  void copy(storage<R, Args...>& to) const {
    ops->copy(&to, this);
  }

  void move(storage<R, Args...>& to) {
    ops->move(&to, this);
  }

  void destroy() {
    ops->destroy(this);
  }

  R invoke(Args... args) const {
    return ops->invoke(this, std::forward<Args>(args)...);
  }

  type_descriptor<R, Args...> const* ops;
  std::aligned_storage_t<sizeof(void*), alignof(void*)> buffer;
};

template <typename R, typename... Args>
struct type_descriptor {
  using storage_t = storage<R, Args...>;

  R (*invoke)(storage_t const*, Args...);
  void (*copy)(storage_t*, storage_t const*);
  void (*move)(storage_t*, storage_t*);
  void (*destroy)(storage_t*);
};

template <typename R, typename... Args>
type_descriptor<R, Args...> const* get_empty_type_descriptor() {

  using storage_t = storage<R, Args...>;

  static constexpr type_descriptor<R, Args...> descriptor = {
      /* invoke = */ [](storage_t const*, Args...) -> R {
        throw bad_function_call();
      },

      /* copy = */
      [](storage_t* other, storage_t const*) {
        other->ops = get_empty_type_descriptor<R, Args...>();
      },

      /* move = */
      [](storage_t* other, storage_t*) {
        other->ops = get_empty_type_descriptor<R, Args...>();
      },

      /* destroy = */ [](storage_t*) {}};
  return &descriptor;
};

template <typename T, typename = void>
struct object_traits;

template <typename T>
struct object_traits<T, std::enable_if_t<is_small<T>>> {

  template <typename R, typename... Args>
  static type_descriptor<R, Args...> const* get_object_descriptor() noexcept {

    using storage_t = storage<R, Args...>;

    static constexpr type_descriptor<R, Args...> descriptor = {
        /* invoke = */ [](storage_t const* source, Args... args) -> R {
          return source->template get_reference_func<T>()(
              std::forward<Args>(args)...);
        },

        /* copy = */
        [](storage_t* other, storage_t const* source) {
          new (&other->buffer) T(source->template get_reference_func<T>());
          other->ops = source->ops;
        },

        /* move = */
        [](storage_t* other, storage_t* source) {
          new (&other->buffer)
              T(std::move(source->template get_reference_func<T>()));
          other->ops = source->ops;
        },

        /* destroy = */
        [](storage_t* source) {
          source->template get_reference_func<T>().~T();
        }};

    return &descriptor;
  }

  template <typename R, typename... Args>
  static void init(storage<R, Args...>& storage, T&& func) {
    new (&storage.buffer) T(std::move(func));
    storage.ops = get_object_descriptor<R, Args...>();
  }

  template <typename Storage, typename V = std::conditional_t<
                                  std::is_const_v<Storage>, T const, T>>
  static V* as_target(Storage& stg) noexcept {
    return &stg.template get_reference_func<V>();
  }
};

template <typename T>
struct object_traits<T, std::enable_if_t<!is_small<T>>> {
  template <typename R, typename... Args>
  static type_descriptor<R, Args...> const* get_object_descriptor() noexcept {

    using storage_t = storage<R, Args...>;

    static constexpr type_descriptor<R, Args...> descriptor = {
        /* invoke = */
        [](storage_t const* source, Args... args) -> R {
          return (*source->template get_pointer_func<T>())(
              std::forward<Args>(args)...);
        },

        /* copy = */
        [](storage_t* other, storage_t const* source) {
          other->set_function(new T(*source->template get_pointer_func<T>()));
          other->ops = source->ops;
        },

        /* move = */
        [](storage_t* other, storage_t* source) {
          other->set_function(source->template get_pointer_func<T>());
          other->ops = source->ops;
          source->ops = get_empty_type_descriptor<R, Args...>();
        },

        /* destroy = */
        [](storage_t* source) {
          delete source->template get_pointer_func<T>();
        }};

    return &descriptor;
  }

  template <typename R, typename... Args>
  static void init(storage<R, Args...>& storage, T&& func) {
    storage.set_function(new T(std::move(func)));
    storage.ops = get_object_descriptor<R, Args...>();
  }

  template <typename Storage, typename V = std::conditional_t<
                                  std::is_const_v<Storage>, T const, T>>
  static V* as_target(Storage& stg) noexcept {
    return stg.template get_pointer_func<V>();
  }
};
}
