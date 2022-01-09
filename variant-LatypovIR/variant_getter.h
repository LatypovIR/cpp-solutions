#pragma once

#include <cstddef>
#include <type_traits>

#include "bad_variant_access.h"
#include "some_type_traits.h"
#include "variant_alternative.h"
#include "variant_union.h"

namespace details {

template <size_t Index, bool _, typename... Types>
constexpr auto& get_data(in_place_index_t<Index>, variant_union<_, Types...>& a) {
  if constexpr (Index == 0) {
    return a.data;
  } else {
    return get_data(in_place_index<Index - 1>, a.tail);
  }
}

template <size_t Index, bool _, typename... Types>
constexpr auto const& get_data(in_place_index_t<Index>, variant_union<_, Types...> const& a) {
  if constexpr (Index == 0) {
    return a.data;
  } else {
    return get_data(in_place_index<Index - 1>, a.tail);
  }
}

} // namespace details

template <typename... Types>
struct variant;

template <size_t Index, typename... Types>
requires(Index < sizeof...(Types)) constexpr decltype(auto) get(variant<Types...>& v) {
  if (v.index() != Index) {
    throw bad_variant_access();
  }
  return details::get_data(in_place_index<Index>, v.storage.union_storage);
}

template <size_t Index, typename... Types>
requires(Index < sizeof...(Types)) constexpr decltype(auto) get(variant<Types...> const& v) {
  if (v.index() != Index) {
    throw bad_variant_access();
  }
  return details::get_data(in_place_index<Index>, v.storage.union_storage);
}

template <size_t Index, typename... Types>
requires(Index < sizeof...(Types)) constexpr decltype(auto) get(variant<Types...>&& v) {
  if (v.index() != Index) {
    throw bad_variant_access();
  }
  return std::move(details::get_data(in_place_index<Index>, v.storage.union_storage));
}

template <size_t Index, typename... Types>
requires(Index < sizeof...(Types)) constexpr decltype(auto) get(variant<Types...> const&& v) {
  if (v.index() != Index) {
    throw bad_variant_access();
  }
  return std::move(details::get_data(in_place_index<Index>, v.storage.union_storage));
}

template <typename T, typename... Types>
requires details::is_unique_v<T, Types...> constexpr T& get(variant<Types...>& v) {
  return get<details::get_index_by_type_v<T, Types...>>(v);
}

template <typename T, typename... Types>
requires details::is_unique_v<T, Types...> constexpr T const& get(variant<Types...> const& v) {
  return get<details::get_index_by_type_v<T, Types...>>(v);
}

template <typename T, typename... Types>
requires details::is_unique_v<T, Types...> constexpr T&& get(variant<Types...>&& v) {
  return get<details::get_index_by_type_v<T, Types...>>(std::move(v));
}

template <typename T, typename... Types>
requires details::is_unique_v<T, Types...> constexpr T const&& get(variant<Types...> const&& v) {
  return get<details::get_index_by_type_v<T, Types...>>(std::move(v));
}

template <size_t Index, typename... Types>
constexpr std::add_pointer_t<variant_alternative_t<Index, variant<Types...>>>
get_if(variant<Types...>* v) noexcept {
  if (!v || v->index() != Index) {
    return nullptr;
  }
  return std::addressof(get<Index>(*v));
}

template <size_t Index, typename... Types>
constexpr std::add_pointer_t<variant_alternative_t<Index, variant<Types...>>> const
get_if(variant<Types...> const* v) noexcept {
  if (!v || v->index() != Index) {
    return nullptr;
  }
  return std::addressof(get<Index>(*v));
}

template <typename T, typename... Types>
constexpr std::add_pointer_t<T> get_if(variant<Types...>* v) noexcept {
  if (!v || v->index() != details::get_index_by_type_v<T, Types...>) {
    return nullptr;
  }
  return std::addressof(get<T>(*v));
}

template <typename T, typename... Types>
constexpr std::add_pointer_t<T const> get_if(variant<Types...> const* v) noexcept {
  if (v == nullptr || v->index() != details::get_index_by_type_v<T const, Types...>) {
    return nullptr;
  }
  return std::addressof(get<T const>(*v));
}