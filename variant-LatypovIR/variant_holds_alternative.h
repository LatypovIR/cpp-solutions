#pragma once

#include "some_type_traits.h"

template <typename... Types>
struct variant;

template <typename T, typename... Types>
constexpr bool holds_alternative(variant<Types...> const& v) noexcept {
  return v.index() == details::get_index_by_type_v<T, Types...>;
};
