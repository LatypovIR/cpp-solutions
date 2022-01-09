#pragma once

#include <cstddef>
#include <forward_list>
#include <type_traits>

template <size_t>
struct in_place_index_t {};

template <size_t index>
static constexpr in_place_index_t<index> in_place_index{};

template <typename T>
struct in_place_type_t {};

template <typename T>
static constexpr in_place_type_t<T> in_place_type{};

namespace details {

template <typename T>
struct not_in_place_tag {
  static constexpr bool value = true;
};

template <typename T>
struct not_in_place_tag<in_place_type_t<T>> {
  static constexpr bool value = false;
};

template <size_t Index>
struct not_in_place_tag<in_place_index_t<Index>> {
  static constexpr bool value = false;
};

template <typename T>
static constexpr bool not_in_place_tag_v = not_in_place_tag<T>::value;

template <bool trivial, typename... Types>
struct variant_union;

template <bool trivial>
struct variant_union<trivial> { constexpr void destroy(size_t) {} };

template <typename T, typename... Types>
struct variant_union<false, T, Types...> {

  using data_t = T;
  using tail_t = variant_union<false, Types...>;

  constexpr explicit variant_union() noexcept {}

  template <typename... Args>
  constexpr explicit variant_union(in_place_index_t<0>, Args&&... args) : data(std::forward<Args>(args)...) {}

  template <size_t N, typename... Args>
  requires(N <= sizeof...(Types)) constexpr explicit variant_union(in_place_index_t<N>, Args&&... args)
      : tail(in_place_index<N - 1>, std::forward<Args>(args)...) {}

  constexpr void destroy(size_t position) {
    if (position == 0) {
      data.~data_t();
    } else {
      tail.destroy(position - 1);
    }
  }

  constexpr ~variant_union(){/*nothing*/};

  union {
    data_t data;
    tail_t tail;
  };
};

template <typename T, typename... Types>
struct variant_union<true, T, Types...> {

  using data_t = T;
  using tail_t = variant_union<true, Types...>;

  constexpr explicit variant_union() noexcept {}

  template <typename... Args>
  constexpr explicit variant_union(in_place_index_t<0>, Args&&... args) : data(std::forward<Args>(args)...) {}

  template <size_t N, typename... Args>
  requires(N <= sizeof...(Types)) constexpr explicit variant_union(in_place_index_t<N>, Args&&... args)
      : tail(in_place_index<N - 1>, std::forward<Args>(args)...) {}

  constexpr ~variant_union() = default;

  union {
    data_t data;
    tail_t tail;
  };
};

template <bool trivial_destroy, typename... Types>
struct variant_storage;

template <typename... Types>
struct variant_storage<false, Types...> {

  constexpr explicit variant_storage() = default;

  template <size_t Index, typename... Args>
  constexpr explicit variant_storage(in_place_index_t<Index>, Args&&... args)
      : position(Index), union_storage(in_place_index<Index>, std::forward<Args>(args)...) {}

  constexpr variant_storage(variant_storage&&) = default;                 // for trivial constr in variant
  constexpr variant_storage(variant_storage const&) = default;            // for trivial constr in variant
  constexpr variant_storage& operator=(variant_storage&&) = default;      // for trivial assign in variant
  constexpr variant_storage& operator=(variant_storage const&) = default; // for trivial assign in variant

  constexpr ~variant_storage() {
    if (position < sizeof...(Types)) {
      union_storage.destroy(position);
    }
  }

  size_t position = 0;
  variant_union<false, Types...> union_storage;
};

template <typename... Types>
struct variant_storage<true, Types...> {

  constexpr explicit variant_storage() = default;

  template <size_t Index, typename... Args>
  constexpr explicit variant_storage(in_place_index_t<Index>, Args&&... args)
      : position(Index), union_storage(in_place_index<Index>, std::forward<Args>(args)...) {}

  constexpr variant_storage(variant_storage&&) = default;                 // for trivial constr in variant
  constexpr variant_storage(variant_storage const&) = default;            // for trivial constr in variant
  constexpr variant_storage& operator=(variant_storage&&) = default;      // for trivial assign in variant
  constexpr variant_storage& operator=(variant_storage const&) = default; // for trivial assign in variant

  constexpr ~variant_storage() = default;

  size_t position = 0;
  details::variant_union<true, Types...> union_storage;
};

template <typename... Types>
using variant_storage_t = variant_storage<(std::is_trivially_destructible_v<Types> && ...), Types...>;

} // namespace details
