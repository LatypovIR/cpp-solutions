#pragma once

#include <cstddef>
#include <type_traits>

template <typename... Types>
struct variant;

namespace details {
template <size_t index, typename... Types>
struct alternative;

template <typename Type0, typename... Types>
struct alternative<0, Type0, Types...> {
  using type = Type0;
};

template <size_t index, typename Type0, typename... Types>
requires (index <= sizeof...(Types))
struct alternative<index, Type0, Types...> {
  using type = typename alternative<index - 1, Types...>::type;
};

template <size_t index, typename... Types>
using alternative_t = typename alternative<index, Types...>::type;
} // namespace details

template <size_t Index, typename T>
struct variant_alternative;

template <size_t Index, typename... Types>
struct variant_alternative<Index, variant<Types...>> {
  using type = details::alternative_t<Index, Types...>;
};

template <size_t Index, typename... Types>
struct variant_alternative<Index, const variant<Types...>> {
  using type = const details::alternative_t<Index, Types...>;
};

template <size_t Index, typename Variant>
using variant_alternative_t = typename variant_alternative<Index, Variant>::type;
