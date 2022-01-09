#pragma once

#include "bad_variant_access.h"
#include "variant_size.h"
#include <cstddef>
#include <type_traits>

// visit like std::visit

template <typename... Types>
struct variant;

namespace details {

template <typename F, size_t... Is>
struct multi_array {

  constexpr F get_visit_function() const {
    return visitor;
  }

  F visitor;
};

template <typename F, size_t I, size_t... Is>
struct multi_array<F, I, Is...> {

  template <typename... Args> // args <=> size_t
  constexpr F get_visit_function(size_t index, Args&&... args) const {
    return arr[index].get_visit_function(std::forward<Args>(args)...);
  }

  multi_array<F, Is...> arr[I];
};

template <typename T>
struct gen_table;

template <typename R, typename V, typename... Vs>
struct gen_table<multi_array<R (*)(V, Vs...)>> {

  template <size_t... Indexes>
  requires(sizeof...(Indexes) == sizeof...(Vs)) static constexpr R visitor(V&& visitor, Vs&&... variants) {
    return std::forward<V>(visitor)(get<Indexes>(std::forward<Vs>(variants))...);
  }

  template <size_t... Indexes>
  requires(sizeof...(Indexes) == sizeof...(Vs)) static constexpr multi_array<R (*)(V, Vs...)> init() {
    return multi_array<R (*)(V, Vs...)>{&visitor<Indexes...>};
  }

  static constexpr multi_array<R (*)(V, Vs...)> table = init<>();
};

template <typename R, typename V, typename... Vs, size_t I, size_t... Is>
struct gen_table<multi_array<R (*)(V, Vs...), I, Is...>> {

  template <size_t... Indexes>
  requires(sizeof...(Indexes) + 1 + sizeof...(Is) ==
           sizeof...(Vs)) static constexpr multi_array<R (*)(V, Vs...), I, Is...> init() {
    return init_indexes<Indexes...>(std::make_index_sequence<I>());
  }

  template <size_t... Indexes, size_t... NIndex>
  requires(sizeof...(NIndex) == I) static constexpr multi_array<R (*)(V, Vs...), I, Is...> init_indexes(
      std::index_sequence<NIndex...>) {
    return multi_array<R (*)(V, Vs...), I, Is...>{
        gen_table<multi_array<R (*)(V, Vs...), Is...>>::template init<Indexes..., NIndex>()...};
  }

  static constexpr multi_array<R (*)(V, Vs...), I, Is...> table = init<>();
};

template <typename Visitor, typename... Variants>
static constexpr decltype(auto) visit(Visitor&& visitor, Variants&&... variants) {
  if ((variants.valueless_by_exception() || ...)) {
    throw bad_variant_access();
  }
  using Return_type = std::invoke_result_t<Visitor&&, decltype(get<0>(std::declval<Variants>()))...>;
  using F = Return_type (*)(Visitor&&, Variants && ...);
  using Multi_array = multi_array<F, variant_size_v<Variants>...>;

  constexpr Multi_array const& v_table = gen_table<Multi_array>::table;
  auto* func = v_table.get_visit_function(variants.index()...);

  return func(std::forward<Visitor>(visitor), std::forward<Variants>(variants)...);
}

} // namespace details
