#pragma once

#include <type_traits>

namespace details {

template <typename T, typename... Types>
struct get_index_by_type;

template <typename T, typename... Types>
struct get_index_by_type<T, T, Types...> {
  static constexpr size_t value = 0;
};

template <typename T, typename Type, typename... Types>
struct get_index_by_type<T, Type, Types...> {
  static constexpr size_t value = get_index_by_type<T, Types...>::value + 1;
};

template <typename T, typename... Types>
inline constexpr size_t get_index_by_type_v = get_index_by_type<T, Types...>::value;

template <typename T, typename... Types>
struct is_unique;

template <typename T>
struct is_unique<T> {
  static constexpr bool value = false;
};

template <typename T, typename... Types>
struct is_unique<T, T, Types...> {
  static constexpr bool value = (!std::is_same_v<T, Types> && ...);
};

template <typename T, typename Type, typename... Types>
requires(!std::is_same_v<Type, T>)
struct is_unique<T, Type, Types...> {
  static constexpr bool value = is_unique<T, Types...>::value;
};

template <typename T, typename... Types>
inline constexpr bool is_unique_v = is_unique<T, Types...>::value;



template <typename A, typename B>
concept Narrow = requires() {
  new A[1]{{std::declval<B>()}};
};

template <typename T, typename Type>
struct function_generator {
  static Type function(Type) requires Narrow<Type, T>{};
};

template <typename T, typename... Types>
struct functions_generator;

template <typename T, typename Type>
struct functions_generator<T, Type> : function_generator<T, Type> {
  using function_generator<T, Type>::function;
};

template <typename T, typename Type, typename... Types>
requires(!std::is_same_v<Type, Types> && ...) struct functions_generator<T, Type, Types...>
    : function_generator<T, Type>, functions_generator<T, Types...> {
  using function_generator<T, Type>::function;
  using functions_generator<T, Types...>::function;
};

template <typename T, typename Type, typename... Types>
requires(std::is_same_v<Type, Types> || ...) struct functions_generator<T, Type, Types...>
    : functions_generator<T, Types...> {
  using functions_generator<T, Types...>::function;
};

template <typename T, typename... Types>
using convertable_t = decltype(functions_generator<T, Types...>::function(std::declval<T>()));

} // namespace details
