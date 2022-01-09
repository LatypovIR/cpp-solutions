#pragma once

#include <cstddef>

#include "variant_alternative.h"
#include "variant_getter.h"
#include "variant_holds_alternative.h"
#include "variant_size.h"
#include "variant_union.h"
#include "variant_visitor.h"

inline constexpr size_t variant_npos = -1;

namespace details {
template <typename F>
static constexpr auto lambda_wrapper(F&& f) {
  using R = std::invoke_result_t<F, int&, int&>;
  return [f](auto&& a, auto&& b) -> R {
    using A = decltype(a);
    using B = decltype(b);
    if constexpr (std::is_same_v<std::remove_const_t<std::remove_reference_t<A>>,
                                 std::remove_const_t<std::remove_reference_t<B>>>) {
      return f(std::forward<A>(a), std::forward<B>(b));
    } else {
      throw details::unreachable_throw();
    }
  };
}
} // namespace details

using details::visit;

template <typename... Types>
struct variant {
  static_assert(sizeof...(Types) > 0, "variant must have at least one alternative");
  static_assert(!(std::is_reference_v<Types> || ...), "variant must have no reference alternative");
  static_assert(!(std::is_void_v<Types> || ...), "variant must have no void alternative");

private:
  template <size_t index>
  using type = details::alternative_t<index, Types...>;

  template <typename T>
  static constexpr size_t position = details::get_index_by_type_v<T, Types...>;

public:
  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Default constructors <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  constexpr variant() noexcept(std::is_nothrow_default_constructible_v<type<0>>)
      requires(std::is_default_constructible_v<type<0>>)
      : storage(in_place_index<0>){};

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Copy constructors <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  static constexpr bool deleted_copy_constructor = !(std::is_copy_constructible_v<Types> && ...);

  static constexpr bool default_copy_constructor = (std::is_trivially_copy_constructible_v<Types> && ...);

  constexpr variant(variant const& other) requires deleted_copy_constructor = delete;

  constexpr variant(variant const& other) noexcept requires default_copy_constructor = default;

  constexpr variant(variant const& other) requires(!deleted_copy_constructor && !default_copy_constructor) {
    if (other.valueless_by_exception()) {
      storage.position = variant_npos;
      return;
    }
    copy_ctor(other);
  };

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Move constructors <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  static constexpr bool deleted_move_constructor = !(std::is_move_constructible_v<Types> && ...);

  static constexpr bool default_move_constructor = (std::is_trivially_move_constructible_v<Types> && ...);

  constexpr variant(variant&& other) requires deleted_move_constructor = delete;

  constexpr variant(variant&& other) noexcept requires default_move_constructor = default;

  constexpr variant(variant&& other) noexcept((std::is_nothrow_move_constructible_v<Types> && ...))
      requires(!deleted_move_constructor && !default_move_constructor) {
    if (other.valueless_by_exception()) {
      storage.position = variant_npos;
      return;
    }
    move_ctor(other);
  };

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Copy assign constructors <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  static constexpr bool deleted_copy_assign =
      !(std::is_copy_constructible_v<Types> && ...) || !(std::is_copy_assignable_v<Types> && ...);

  static constexpr bool default_copy_assign = (std::is_trivially_copy_constructible_v<Types> && ...) &&
                                              (std::is_trivially_copy_assignable_v<Types> && ...) &&
                                              (std::is_trivially_destructible_v<Types> && ...);

  constexpr variant& operator=(variant const& other) requires deleted_copy_assign = delete;

  constexpr variant& operator=(variant const& other) noexcept requires default_copy_assign = default;

  constexpr variant& operator=(variant const& other) requires(!deleted_copy_assign && !default_copy_assign) {

    if (this == &other || (valueless_by_exception() && other.valueless_by_exception())) {
      return *this;
    }

    if (index() == other.index()) {
      details::visit(details::lambda_wrapper([&](auto& a, auto const& b) { a = b; }), *this, other);
      return *this;
    }

    if (!valueless_by_exception()) {
      destroy();
    }

    if (!other.valueless_by_exception()) {
      copy_ctor(other);
    }

    return *this;
  }

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Move assign constructors <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  static constexpr bool deleted_move_assign =
      !(std::is_move_constructible_v<Types> && ...) || !((std::is_move_assignable_v<Types>)&&...);

  static constexpr bool default_move_assign = (std::is_trivially_move_constructible_v<Types> && ...) &&
                                              (std::is_trivially_move_assignable_v<Types> && ...) &&
                                              (std::is_trivially_destructible_v<Types> && ...);

  constexpr variant& operator=(variant&& other) requires deleted_move_assign = delete;

  constexpr variant& operator=(variant&& other) noexcept requires default_move_assign = default;

  constexpr variant& operator=(variant&& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                           std::is_nothrow_move_assignable_v<Types>)&&...))
      requires(!deleted_move_assign && !default_move_assign) {
    if (this == &other || (valueless_by_exception() && other.valueless_by_exception())) {
      return *this;
    }

    if (index() == other.index()) {
      details::visit(details::lambda_wrapper([&](auto& a, auto&& b) { a = std::move(b); }), *this, other);
      return *this;
    }

    if (!valueless_by_exception()) {
      destroy();
    }

    if (!other.valueless_by_exception()) {
      move_ctor(other);
    }

    return *this;
  };

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> In place constructors <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  template <size_t Index, typename... Args>
  constexpr explicit variant(in_place_index_t<Index>,
                             Args&&... args) noexcept(std::is_nothrow_constructible_v<type<Index>, Args...>)
      requires(Index < sizeof...(Types) && std::is_constructible_v<type<Index>, Args...>)
      : storage(in_place_index<Index>, std::forward<Args>(args)...) {}

  template <typename T, typename... Args>
  constexpr explicit variant(in_place_type_t<T>,
                             Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
      requires(std::is_constructible_v<T, Args...>&& details::is_unique_v<T, Types...>)
      : variant(in_place_index<position<T>>, std::forward<Args>(args)...) {}

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Convert constructor <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  template <typename T, typename Type = details::convertable_t<T, Types...>>
  requires(details::not_in_place_tag_v<std::remove_reference_t<T>>&& std::is_convertible_v<T, Type>&&
               details::is_unique_v<
                   Type, Types...>) constexpr variant(T&& value) noexcept(std::is_nothrow_convertible_v<T, Type>)
      : variant(in_place_type<Type>, std::forward<T>(value)) {}

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Convert operator <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  template <typename T, typename Type = details::convertable_t<T, Types...>>
  requires(details::not_in_place_tag_v<std::remove_cvref_t<T>>&& std::is_convertible_v<T, Type>&&
               details::is_unique_v<Type, Types...>) constexpr variant&
  operator=(T&& value) noexcept(
      std::is_nothrow_assignable_v<Type&, T>&& std::is_nothrow_constructible_v<Type, T>) {
    if (index() == position<Type>) {
      get<Type>(*this) = std::forward<T>(value);
      return *this;
    }

    if constexpr (std::is_nothrow_constructible_v<Type, T> || !std::is_nothrow_move_constructible_v<Type>) {
      emplace<position<Type>>(std::forward<T>(value));
    } else {
      emplace<position<Type>>(Type(std::forward<T>(value)));
    }

    return *this;
  }

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Other <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  constexpr size_t index() const noexcept {
    return storage.position;
  }

  constexpr bool valueless_by_exception() const noexcept {
    return index() == variant_npos;
  }

  constexpr void swap(variant& other) noexcept(((std::is_nothrow_move_constructible_v<Types> &&
                                                 std::is_nothrow_swappable_v<Types>)&&...))
      requires((std::is_swappable_v<Types> && ...) && (std::is_move_constructible_v<Types> && ...)) {
    if ((valueless_by_exception() && other.valueless_by_exception()) || this == &other) {
      return;
    }
    if (index() == other.index()) {
      details::visit(details::lambda_wrapper([](auto& a, auto& b) {
                     using std::swap;
                     swap(a, b);
                   }),
                   *this, other);

    } else {
      variant tmp = std::move(*this);
      *this = std::move(other);
      other = std::move(tmp);
    }
  }

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Emplacement <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  template <size_t Index, typename... Args>
  requires(std::is_constructible_v<type<Index>, Args...>) constexpr void emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<type<Index>, Args...>) {
    if (!valueless_by_exception()) {
      destroy();
      storage.position = variant_npos;
    }

    using T = type<Index>;
    auto* ptr = std::addressof(get_data(in_place_index<Index>, storage.union_storage));
    new (const_cast<std::remove_const_t<T>*>(ptr)) T(std::forward<Args>(args)...);
    storage.position = Index;
  }

  template <typename T, typename... Args>
  requires(std::is_constructible_v<T, Args...>) constexpr void emplace(Args&&... args) noexcept(
      std::is_nothrow_constructible_v<T, Args...>) {
    emplace<position<T>>(std::forward<Args>(args)...);
  }

  /**>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Compare operators <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<**/

  constexpr friend bool operator==(variant const& v, variant const& w) {
    if (v.index() != w.index()) {
      return false;
    }
    if (v.valueless_by_exception()) {
      return true;
    }
    return details::visit(details::lambda_wrapper([](auto&& a, auto&& b) { return a == b; }), v, w);
  }

  constexpr friend bool operator!=(variant const& v, variant const& w) {
    if (v.index() != w.index()) {
      return true;
    }
    if (v.valueless_by_exception()) {
      return false;
    }
    return details::visit(details::lambda_wrapper([](auto&& a, auto&& b) { return a != b; }), v, w);
  }

  constexpr friend bool operator<(variant const& v, variant const& w) {
    if (w.valueless_by_exception()) {
      return false;
    }
    if (v.valueless_by_exception()) {
      return true;
    }
    if (v.index() != w.index()) {
      return v.index() < w.index();
    }
    return details::visit(details::lambda_wrapper([](auto&& a, auto&& b) { return a < b; }), v, w);
  }

  constexpr friend bool operator>(variant const& v, variant const& w) {
    if (v.valueless_by_exception()) {
      return false;
    }
    if (w.valueless_by_exception()) {
      return true;
    }
    if (v.index() != w.index()) {
      return v.index() > w.index();
    }
    return details::visit(details::lambda_wrapper([](auto&& a, auto&& b) { return a > b; }), v, w);
  }

  constexpr friend bool operator<=(variant const& v, variant const& w) {
    if (v.valueless_by_exception()) {
      return true;
    }
    if (w.valueless_by_exception()) {
      return false;
    }
    if (v.index() != w.index()) {
      return v.index() < w.index();
    }
    return details::visit(details::lambda_wrapper([](auto&& a, auto&& b) { return a <= b; }), v, w);
  }

  constexpr friend bool operator>=(variant const& v, variant const& w) {
    if (w.valueless_by_exception()) {
      return true;
    }
    if (v.valueless_by_exception()) {
      return false;
    }
    if (v.index() != w.index()) {
      return v.index() > w.index();
    }
    return details::visit(details::lambda_wrapper([](auto&& a, auto&& b) { return a >= b; }), v, w);
  }

private:
  details::variant_storage_t<Types...> storage;

  constexpr void copy_ctor(variant const& other) {
    storage.position = other.index();
    details::visit(details::lambda_wrapper([&](auto& a, auto&& b) {
                   using T = std::remove_reference_t<decltype(a)>;
                   storage.position = variant_npos;
                   new (std::addressof(a)) T(b);
                   storage.position = other.index();
                 }),
                 *this, other);
  }

  constexpr void move_ctor(variant& other) {
    storage.position = other.index();
    details::visit(details::lambda_wrapper([&](auto& a, auto&& b) {
                   using T = std::remove_reference_t<decltype(a)>;
                   storage.position = variant_npos;
                   new (std::addressof(a)) T(std::move(b));
                   storage.position = other.index();
                 }),
                 *this, other);
  }

  constexpr void destroy() {
    details::visit(
        [](auto& a) {
          using T = std::remove_reference_t<decltype(a)>;
          a.~T();
        },
        *this);
    storage.position = variant_npos;
  }

  template <size_t Index, typename... T>
  requires(Index < sizeof...(T)) friend constexpr decltype(auto) get(variant<T...>& v);

  template <size_t Index, typename... T>
  requires(Index < sizeof...(T)) friend constexpr decltype(auto) get(variant<T...>&& v);

  template <size_t Index, typename... T>
  requires(Index < sizeof...(T)) friend constexpr decltype(auto) get(variant<T...> const& v);

  template <size_t Index, typename... T>
  requires(Index < sizeof...(T)) friend constexpr decltype(auto) get(variant<T...> const&& v);
};
