#pragma once

#include <cstddef>
#include <type_traits>

template <typename... Types>
struct variant;

template <typename T>
struct variant_size;

template <typename... Types>
struct variant_size<variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename... Types>
struct variant_size<const variant<Types...>> : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename Variant>
static constexpr size_t variant_size_v = variant_size<std::remove_reference_t<Variant>>::value;
