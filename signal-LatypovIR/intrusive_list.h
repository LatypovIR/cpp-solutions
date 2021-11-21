#pragma once

#include <iterator>

using namespace std;

namespace intrusive {
/*
Тег по-умолчанию чтобы пользователям не нужно было
придумывать теги, если они используют лишь одну базу
list_element.
*/
struct default_tag;

struct base_list_element {

  base_list_element();
  base_list_element(base_list_element&& other);
  base_list_element& operator=(base_list_element&& other);
  ~base_list_element();

  /* Отвязывает элемент из списка в котором он находится. */
  void unlink();
  void insert(base_list_element* other);
  void splice(base_list_element* first, base_list_element* last);

  base_list_element* prev;
  base_list_element* next;

  template <typename U, typename UTag>
  friend struct list;
};

template <typename Tag = default_tag>
struct list_element : private base_list_element {
  list_element(){};
  ~list_element(){};

  void unlink() {
    base_list_element::unlink();
  }

  list_element(list_element const&) = delete;
  list_element& operator=(list_element const&) = delete;

  template <typename U, typename UTag>
  friend struct list;
};

template <typename T, typename Tag = default_tag>
struct list {

private:
  template <typename UT, typename UTag>
  struct base_iterator;

  using node = base_list_element;
  mutable node fake_node;

public:
  using iterator = base_iterator<T, Tag>;
  using const_iterator = base_iterator<T const, Tag>;

  static_assert(std::is_convertible_v<T&, list_element<Tag>&>,
                "value type is not convertible to base_list_element");

  iterator as_iterator(T* node) {
      return iterator(node);
  }

  const_iterator as_iterator(T* node) const {
    return const_iterator (node);
  }

  list() noexcept : fake_node() {
    fake_node.prev = &fake_node;
    fake_node.next = &fake_node;
  }

  list(list const&) = delete;

  list(list&& other) noexcept : list() {
    *this = std::move(other);
  };

  ~list() {
    clear();
  };

  list& operator=(list const&) = delete;

  list& operator=(list&& other) noexcept {
    if (this != &other) {
      clear();
      fake_node = std::move(other.fake_node);
    }
    return *this;
  };

  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  };

  /*
  Поскольку вставка изменяет данные в base_list_element
  мы принимаем неконстантный T&.
  */

  void push_back(T& element) noexcept {
    insert(end(), element);
  }

  void pop_back() noexcept {
    erase(--end());
  };

  T& back() noexcept {
    return *(--end());
  };

  T const& back() const noexcept {
    return *(--end());
  };

  void push_front(T& element) noexcept {
    insert(begin(), element);
  };

  void pop_front() noexcept {
    erase(begin());
  };

  T& front() noexcept {
    return *begin();
  };

  T const& front() const noexcept {
    return *begin();
  };

  bool empty() const noexcept {
    return fake_node.prev == &fake_node;
  };

  iterator begin() noexcept {
    return iterator(fake_node.next);
  };

  const_iterator begin() const noexcept {
    return const_iterator(fake_node.next);
  };

  iterator end() noexcept {
    return iterator(&fake_node);
  };

  const_iterator end() const noexcept {
    return const_iterator(&fake_node);
  };

  iterator insert(const_iterator pos, T& value) noexcept {
    node* val = &static_cast<node&>(static_cast<list_element<Tag>&>(value));
    pos.current->insert(val);
    return iterator(val);
  };

  iterator erase(const_iterator pos) noexcept {
    iterator result(pos.current->next);
    const_cast<node*>(pos.current)->unlink();
    return result;
  };

  void splice(const_iterator pos, list&, const_iterator first,
              const_iterator last) noexcept {
    pos.current->splice(first.current, last.current);
  };
};

template <typename U, typename UTag>
template <typename T, typename Tag>
struct list<U, UTag>::base_iterator {
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = T;
  using reference = T&;
  using pointer = T*;

  base_iterator() = default;
  base_iterator(base_iterator const&) = default;

  template <typename V, typename = std::enable_if_t<std::is_const_v<T> &&
                                                    !std::is_const_v<V>>>
  base_iterator(base_iterator<V, Tag> const& other) : current(other.current) {}

  base_iterator& operator++() {
    current = current->next;
    return *this;
  };

  base_iterator operator++(int) {
    base_iterator copy(*this);
    current = current->next;
    return copy;
  };

  base_iterator& operator--() {
    current = current->prev;
    return *this;
  };

  base_iterator operator--(int) {
    base_iterator copy(*this);
    current = current->prev;
    return copy;
  };

  T& operator*() const {
    return static_cast<T&>(static_cast<list_element<Tag>&>(*current));
  };

  T* operator->() const {
    return &(operator*());
  };

  friend bool operator==(base_iterator const& lhs, base_iterator const& rhs) {
    return lhs.current == rhs.current;
  }

  friend bool operator!=(base_iterator const& lhs, base_iterator const& rhs) {
    return lhs.current != rhs.current;
  }

private:
  node* current;
  explicit base_iterator(node* element) : current(element) {}

  friend struct list<U, UTag>;
};
}