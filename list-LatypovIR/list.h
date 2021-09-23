#pragma once
#include <cassert>
#include <iterator>

template <typename T>
class list {
  struct Node;
  struct FakeNode;
  struct list_iterator;
  struct const_list_iterator;

  template <typename U>
  struct abstract_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = U;
    using pointer = U*;
    using reference = U&;

    abstract_iterator(abstract_iterator const& other) : abstract_iterator(other.node) {};

    reference operator*() const {
      return static_cast<Node*>(node)->value;
    };

    pointer operator->() const {
      return &(static_cast<Node*>(node)->value);
    };

    abstract_iterator& operator++() & {
      node = node->next;
      return *this;
    };

    abstract_iterator operator++(int) & {
      abstract_iterator tmp(node);
      ++(*this);
      return tmp;
    };

    abstract_iterator& operator--() & {
      node = node->prev;
      return *this;
    };

    abstract_iterator operator--(int) & {
      abstract_iterator tmp(node);
      --(*this);
      return tmp;
    };

    friend bool operator==(abstract_iterator const& a, abstract_iterator const& b) {
      return a.node == b.node;
    }

    friend bool operator!=(abstract_iterator const& a, abstract_iterator const& b) {
      return a.node != b.node;
    }

    explicit abstract_iterator(FakeNode* node) : node(node) {};

  protected:
    FakeNode* node;
    friend struct list<T>::const_list_iterator;
    friend struct list<T>::list_iterator;
  };

public:
  // bidirectional iterator
  using iterator = list_iterator;
  // bidirectional iterator
  using const_iterator = const_list_iterator;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;


  // O(1)
  list() noexcept {
    fake.next = fake.prev = &fake;
  };

  // O(n), strong
  list(list const& other) : list() {
    try {
      for (auto it = other.begin(); it != other.end(); ++it) {
        push_back(*it);
      }
    } catch (...) {
      clear();
      throw;
    }
  };

  // O(n), strong
  list& operator=(list const& other) {
    list(other).swap(*this);
    return *this;
  };

  // O(n)
  ~list() {
    clear();
  };

  // O(1)
  bool empty() const noexcept {
    return fake.next == &fake;
  };

  // O(1)
  T& front() noexcept {
    return static_cast<Node*>(fake.next)->value;
  };

  // O(1)
  T const& front() const noexcept {
    return static_cast<Node*>(fake.next)->value;
  };

  // O(1), strong
  void push_front(T const& new_value) {
    Node* new_node = new Node(new_value);
    new_node->next = fake.next;
    new_node->prev = &fake;
    fake.next->prev = new_node;
    fake.next = new_node;
  };

  // O(1)
  void pop_front() noexcept {
    Node* old_node = static_cast<Node*>(fake.next);
    fake.next = fake.next->next;
    fake.next->prev = &fake;
    delete old_node;
  };

  // O(1)
  T& back() noexcept {
    return static_cast<Node*>(fake.prev)->value;
  };

  // O(1)
  T const& back() const noexcept {
    return static_cast<Node*>(fake.prev)->value;
  };

  // O(1), strong
  void push_back(T const& new_value) {
    Node* new_node = new Node(new_value);
    new_node->prev = fake.prev;
    new_node->next = &fake;
    fake.prev->next = new_node;
    fake.prev = new_node;
  };

  // O(1)
  void pop_back() noexcept {
    Node* old_node = static_cast<Node*>(fake.prev);
    fake.prev = fake.prev->prev;
    fake.prev->next = &fake;
    delete old_node;
  };

  // O(1)
  iterator begin() noexcept {
    return iterator(fake.next);
  };

  // O(1)
  const_iterator begin() const noexcept {
    return const_iterator(fake.next);
  };

  // O(1)
  iterator end() noexcept {
    return iterator(&fake);
  };

  // O(1)
  const_iterator end() const noexcept {
    return const_iterator(&fake);
  };

  // O(1)
  reverse_iterator rbegin() noexcept {
    return std::make_reverse_iterator(end());
  };
  // O(1)
  const_reverse_iterator rbegin() const noexcept {
    return std::make_reverse_iterator(end());
  };

  // O(1)
  reverse_iterator rend() noexcept {
    return std::make_reverse_iterator(begin());
  };
  // O(1)
  const_reverse_iterator rend() const noexcept {
    return std::make_reverse_iterator(begin());
  };

  // O(n)
  void clear() noexcept {
    while (!empty()) {
      pop_back();
    }
  };

  // O(1), strong
  iterator insert(const_iterator pos, T const& val);
  // O(1)
  iterator erase(const_iterator pos) noexcept;
  // O(n)
  iterator erase(const_iterator first, const_iterator last) noexcept;
  // O(1)
  void splice(const_iterator pos, list& other,
              const_iterator first, const_iterator last) noexcept;

  void swap(list& other) {
    if (empty() && other.empty()) {
      return;
    }
    if (empty()) {
      fake.next = other.fake.next;
      fake.prev = other.fake.prev;
      fake.next->prev = &fake;
      fake.prev->next = &fake;
      other.fake.next = other.fake.next = &other.fake;
      return;
    }

    if (other.empty()) {
      other.swap(*this);
      return;
    }

    std::swap(fake, other.fake);

    fake.next->prev = &fake;
    fake.prev->next = &fake;

    other.fake.next->prev = &other.fake;
    other.fake.prev->next = &other.fake;
  }

  friend void swap(list& a, list& b) noexcept {
    a.swap(b);
  };

private:
  mutable FakeNode fake;
};

template <typename T>
struct list<T>::FakeNode {

  FakeNode() {};

private:
  FakeNode* next;
  FakeNode* prev;
  friend struct list;
};

template <typename T>
struct list<T>::Node : FakeNode {
  T value;
  Node(T const& element) : value(element) {}
};

template<typename T>
struct list<T>::list_iterator : abstract_iterator<T> {
  list_iterator(abstract_iterator<T> const& other) : abstract_iterator<T>(other.node) {};

private:
  list_iterator(list<T>::FakeNode* node) : abstract_iterator<T>(node) {};
  friend class list;
};

template<typename T>
struct list<T>::const_list_iterator : abstract_iterator<T const> {

  const_list_iterator(abstract_iterator<T const> const& other) : abstract_iterator<T const>(other.node) {};

  const_list_iterator(list_iterator const& other) : abstract_iterator<T const>(other.node) {};

  friend bool operator==(const_iterator const& a, const_iterator const& b) {
    return a.node == b.node;
  }

  friend bool operator!=(const_iterator const& a, const_iterator const& b) {
    return a.node != b.node;
  }

private:
  const_list_iterator(FakeNode* node) : abstract_iterator<T const>(node) {};
  friend class list;
};


template <typename T>
typename list<T>::iterator list<T>::insert(const_iterator pos, const T& val) {
  Node* new_node = new Node(val);
  FakeNode* old_node = pos.node;

  new_node->next = old_node;
  new_node->prev = old_node->prev;

  old_node->prev->next = new_node;
  old_node->prev = new_node;

  return list_iterator(new_node);
}

template <typename T>
typename list<T>::iterator list<T>::erase(const_iterator pos) noexcept {
  FakeNode* old_node = pos.node;
  iterator it(old_node->next);

  old_node->prev->next = old_node->next;
  old_node->next->prev = old_node->prev;

  delete static_cast<Node*>(old_node);
  return it;
}

template <typename T>
typename list<T>::iterator list<T>::erase(const_iterator first, const_iterator last) noexcept {
  iterator start(first.node);

  while (start != last) {
    start = erase(start);
  };
  return start;
}

template <typename T>
void list<T>::splice(const_iterator pos, list<T>&, const_iterator first, const_iterator last) noexcept {
  if (first == last)return;

  FakeNode* last_in = last.node->prev;

  first.node->prev->next = last.node;
  last.node->prev = first.node->prev;

  pos.node->prev->next = first.node;
  first.node->prev = pos.node->prev;

  pos.node->prev = last_in;
  last_in->next = pos.node;
}
