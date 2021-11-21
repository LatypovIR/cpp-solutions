#pragma once

#include <cstddef>
#include <stdexcept>

#include "intrusive_tree.h"

namespace tree_structs {

struct left_tag;
struct right_tag;

using left_base_node = node_element<left_tag>;
using right_base_node = node_element<right_tag>;

template <typename T, typename Parent>
struct node_with_value : Parent {

  template <typename X>
  node_with_value(X&& value) : value(std::forward<X>(value)) {}

  T value;
};

template <typename Left>
using left_node = node_with_value<Left, left_base_node>;

template <typename Right>
using right_node = node_with_value<Right, right_base_node>;

template <typename Left, typename Right>
struct bimap_node : left_node<Left>, right_node<Right> {

  template <typename X, typename Y>
  bimap_node(X&& left, Y&& right)
      : left_node<Left>(std::forward<X>(left)),
        right_node<Right>(std::forward<Y>(right)) {}

};

struct fake_bimap_node : left_base_node, right_base_node {};

template <typename T, typename Parent, typename Comparator>
struct super_comparator {

  using node_t = node_with_value<T, Parent>;

  super_comparator(Comparator comparator) : comparator(std::move(comparator)) {}

  bool operator()(node_t const& a, node_t const& b) const {
    return comparator(a.value, b.value);
  }

  bool operator()(node_t const& a, T const& b) const {
    return comparator(a.value, b);
  }

  bool operator()(T const& a, node_t const& b) const {
    return comparator(a, b.value);
  }

  bool operator()(T const& a, T const& b) const {
    return comparator(a, b);
  }

private:
  Comparator comparator;
};

template <typename Left, typename Comparator>
using left_comparator = super_comparator<Left, left_base_node, Comparator>;

template <typename Right, typename Comparator>
using right_comparator = super_comparator<Right, right_base_node, Comparator>;
}

template <typename Left, typename Right, typename CompareLeft = std::less<>,
              typename CompareRight = std::less<>>
struct bimap {

private:
  using left_t = Left;
  using right_t = Right;

  using fake_node_t = tree_structs::fake_bimap_node;
  using node_t = tree_structs::bimap_node<Left, Right>;

  using left_base_node = tree_structs::left_base_node;
  using right_base_node = tree_structs::right_base_node;

  using left_comparator = tree_structs::left_comparator<Left, CompareLeft>;
  using right_comparator = tree_structs::right_comparator<Right, CompareRight>;

  using left_node = tree_structs::left_node<Left>;
  using right_node = tree_structs::right_node<Right>;

  using left_tree_t = tree_structs::intrusive_tree
      <left_node, Left, tree_structs::left_tag, left_comparator>;

  using right_tree_t = tree_structs::intrusive_tree
      <right_node, Right, tree_structs::right_tag, right_comparator>;

  using left_iterator_t = typename left_tree_t::iterator;
  using right_iterator_t = typename right_tree_t::iterator;

  fake_node_t fake;
  left_tree_t left_tree;
  right_tree_t right_tree;

  template <bool left>
  struct universal_iterator;

public:
  using left_iterator = universal_iterator<true>;
  using right_iterator = universal_iterator<false>;

private:
  template <bool left>
  struct universal_iterator {

  private:
    using iterator_t =
        std::conditional_t<left, left_iterator_t, right_iterator_t>;

    using other_iterator_t =
        std::conditional_t<!left, left_iterator_t, right_iterator_t>;

    using value_t = std::conditional_t<left, left_t, right_t>;

    using fake_t =
        std::conditional_t<left, left_base_node, right_base_node>;

    using other_fake_t =
        std::conditional_t<!left, left_base_node, right_base_node>;

    using other_node_t =
        std::conditional_t<!left, left_node, right_node>;

    iterator_t it;

    friend struct bimap;

  public:
    universal_iterator(universal_iterator const& other) = default;

    universal_iterator(iterator_t const& it) : it(it) {};

    value_t const& operator*() const {
      return it->value;
    }

    value_t const* operator->() const {
      return &(operator*());
    };

    universal_iterator& operator++() {
      ++it;
      return *this;
    };

    universal_iterator operator++(int) {
      return universal_iterator(it++);
    };

    universal_iterator& operator--() {
      --it;
      return *this;
    }

    universal_iterator operator--(int) {
      return universal_iterator(it--);
    }

    universal_iterator<!left> flip() const {
      if (!it.is_end()) {
        return other_iterator_t(
            static_cast<other_node_t*>(static_cast<node_t*>(&(*it))));
      } else {
        return other_iterator_t(static_cast<other_fake_t*>(
            static_cast<fake_node_t*>(static_cast<fake_t*>(it.node))));
      }
    }

    friend bool operator==(universal_iterator const& a,
                           universal_iterator const& b) {
      return a.it == b.it;
    }

    friend bool operator!=(universal_iterator const& a,
                           universal_iterator const& b) {
      return !(a == b);
    }
  };

public:

  // Создает bimap не содержащий ни одной пары.
  bimap(CompareLeft compare_left = CompareLeft(),
        CompareRight compare_right = CompareRight()) :
        fake(),
        left_tree(static_cast<left_base_node*>(&fake), compare_left),
        right_tree(static_cast<right_base_node*>(&fake), compare_right) {}

  // Конструкторы от других и присваивания
  bimap(bimap const& other) : bimap() {
    for (auto it = other.begin_left(); it != other.end_left(); ++it) {
      insert(*it, *it.flip());
    }
  };

  bimap(bimap&& other) noexcept : bimap() {
    swap(other);
  };

  void swap(bimap& other) {
    left_tree.swap(other.left_tree);
    right_tree.swap(other.right_tree);
  }

  bimap& operator=(bimap const& other) {
    if (this == &other) {
      return *this;
    }
    erase_left(begin_left(), end_left());
    bimap(other).swap(*this);
    return *this;
  };

  bimap& operator=(bimap&& other) noexcept {
    if (this == &other) {
      return *this;
    }

    erase_left(begin_left(), end_left());
    swap(other);

    return *this;
  };

  // Деструктор. Вызывается при удалении объектов bimap.
  // Инвалидирует все итераторы ссылающиеся на элементы этого bimap
  // (включая итераторы ссылающиеся на элементы следующие за последними).
  ~bimap() {
    erase_left(begin_left(), end_left());
  };

  // Вставка пары (left, right), возвращает итератор на left.
  // Если такой left или такой right уже присутствуют в bimap, вставка не
  // производится и возвращается end_left().
  left_iterator insert(left_t const& left, right_t const& right) {
    return insert_(left, right);
  };

  left_iterator insert(left_t const& left, right_t&& right) {
    return insert_(left, std::move(right));
  };

  left_iterator insert(left_t&& left, right_t const& right) {
    return insert_(std::move(left), right);
  };

  left_iterator insert(left_t&& left, right_t&& right) {
    return insert_(std::move(left), std::move(right));
  };

private:
  template <typename X, typename Y>
  left_iterator insert_(X&& left, Y&& right) {
    if (left_tree.find(left) != left_tree.end() ||
        right_tree.find(right) != right_tree.end()) {
      return end_left();
    }

    auto* nd = new node_t(std::forward<X>(left), std::forward<Y>(right));

    right_tree.insert(*nd);
    return left_tree.insert(*nd);
  }

public:
  // Удаляет элемент и соответствующий ему парный.
  // erase невалидного итератора неопределен.
  // erase(end_left()) и erase(end_right()) неопределены.
  // Пусть it ссылается на некоторый элемент e.
  // erase инвалидирует все итераторы ссылающиеся на e и на элемент парный к e.
  left_iterator erase_left(left_iterator it) {
    auto next_it = upper_bound_left(*it);
    erase_left(*it);
    return next_it;
  }
  // Аналогично erase, но по ключу, удаляет элемент если он присутствует, иначе
  // не делает ничего Возвращает была ли пара удалена
  bool erase_left(left_t const& left) {
    auto l_it = find_left(left);
    if (l_it == end_left()) {
      return false;
    }
    left_tree.erase(left);
    right_tree.erase(*l_it.flip());

    delete static_cast<node_t*>(&(*(l_it.it)));

    return true;
  }

  right_iterator erase_right(right_iterator it) {
    auto next_it = upper_bound_right(*it);
    erase_right(*it);
    return next_it;
  }

  bool erase_right(right_t const& right) {
    auto r_it = find_right(right);
    if (r_it == end_right()) {
      return false;
    }
    left_tree.erase(*r_it.flip());
    right_tree.erase(right);

    delete static_cast<node_t*>(&(*(r_it.it)));
    return true;
  }

  // erase от ренжа, удаляет [first, last), возвращает итератор на последний
  // элемент за удаленной последовательностью
  left_iterator erase_left(left_iterator first, left_iterator last) {
    while (first != last) {
      first = erase_left(first);
    }

    return last;
  }

  right_iterator erase_right(right_iterator first, right_iterator last) {
    while (first != last) {
      first = erase_right(first);
    }

    return last;
  }

  // Возвращает итератор по элементу. Если не найден - соответствующий end()
  left_iterator find_left(left_t const& left) const {
    return left_tree.find(left);
  };

  right_iterator find_right(right_t const& right) const {
    return right_tree.find(right);
  };

  left_t const& at_right(right_t const& key) const {
    auto it = find_right(key);
    if (it == end_right()) {
      throw std::out_of_range("out of range for function at_right\n");
    }
    return *(it.flip());
  };

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует, добавляет его в bimap и на противоположную
  // сторону кладет дефолтный элемент, ссылку на который и возвращает
  // Если дефолтный элемент уже лежит в противоположной паре - должен поменять
  // соответствующий ему элемент на запрашиваемый (смотри тесты)
  template <typename = std::is_default_constructible<right_t>>
  right_t const& at_left_or_default(left_t const& key) {
    auto it = find_left(key);
    if (it != end_left()) {
      return *it.flip();
    }
    right_t new_default_right = right_t();
    auto it2 = find_right(new_default_right);

    if (it2 != end_right()) {
      erase_right(it2);
    }

    return *(insert(key, new_default_right).flip());
  }

  // Возвращает противоположный элемент по элементу
  // Если элемента не существует -- бросает std::out_of_range
  right_t const& at_left(left_t const& key) const {
    auto it = find_left(key);
    if (it == end_left()) {
      throw std::out_of_range("out of range for function at_left\n");
    }
    return *(it.flip());
  };

  template <typename = std::is_default_constructible<left_t>>
  left_t const& at_right_or_default(right_t const& key) {
    auto it = find_right(key);
    if (it != end_right()) {
      return *it.flip();
    }
    left_t new_default_left = left_t();
    auto it2 = find_left(new_default_left);

    if (it2 != end_left()) {
      erase_left(it2);
    }

    return *insert(new_default_left, key);
  }

  // lower и upper bound'ы по каждой стороне
  // Возвращают итераторы на соответствующие элементы
  // Смотри std::lower_bound, std::upper_bound.
  left_iterator lower_bound_left(const left_t& left) const {
    return left_tree.lower_bound(left);
  }

  left_iterator upper_bound_left(const left_t& left) const {
    return left_tree.upper_bound(left);
  }

  right_iterator lower_bound_right(const right_t& right) const {
    return right_tree.lower_bound(right);
  }

  right_iterator upper_bound_right(const right_t& right) const {
    return right_tree.upper_bound(right);
  }

  // Возващает итератор на минимальный по порядку left.
  left_iterator begin_left() const {
    return left_iterator(left_tree.begin());
  }

  // Возващает итератор на следующий за последним по порядку left.
  left_iterator end_left() const {
    return left_iterator(left_tree.end());
  }

  // Возващает итератор на минимальный по порядку right.
  right_iterator begin_right() const {
    return right_iterator(right_tree.begin());
  }

  // Возващает итератор на следующий за последним по порядку right.
  right_iterator end_right() const {
    return right_iterator(right_tree.end());
  };

  // Проверка на пустоту
  bool empty() const {
    return left_tree.empty();
  };

  // Возвращает размер бимапы (кол-во пар)
  std::size_t size() const {
    return left_tree.size();
  };

  // операторы сравнения
  friend bool operator==(bimap const& a, bimap const& b) {
    if (a.size() != b.size()) {
      return false;
    }

    auto other = b.begin_left();
    for (auto it = a.begin_left(); it != a.end_left(); ++it, ++other) {
      if (!a.equals_values(it, other)) {
        return false;
      }
    }
    return true;
  }

  friend bool operator!=(bimap const& a, bimap const& b) {
    return !(a == b);
  };

private:
  bool equals_values(left_iterator const& a, left_iterator const& b) const {
    return !left_tree.get_comparator()(*a, *b) &&
           !left_tree.get_comparator()(*b, *a) &&
           !right_tree.get_comparator()(*a.flip(), *b.flip()) &&
           !right_tree.get_comparator()(*b.flip(), *a.flip());
  }
};
