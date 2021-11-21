#pragma once

#include <type_traits>

#include "default_tree.h"

namespace tree_structs {

// ContainerType : node_element<Tag>

// Comparator(Container const&, Container const&) &&
// Comparator(Container const&, T const&) &&
// Comparator(T const&, Container const&)

template <typename Container, typename Key, typename Tag, typename Comparator>
struct intrusive_tree {

private:
  default_tree<Container, Key, Tag, Comparator> tree;
  base_node_element* begin_;
  base_node_element* fake; // fake->left == root

  void update() {
    fake->left = tree.root();
    begin_ = fake;

    while (begin_->left) {
      begin_ = begin_->left;
    }

    if (fake->left) {
      fake->left->parent = fake;
    }
  }

public:
  struct iterator {

    iterator(Container* container)
        : iterator(static_cast<base_node_element*>(
              static_cast<node_element<Tag>*>(container))){};

    iterator(base_node_element* node) : node(node) {};

    Container& operator*() const {
      return static_cast<Container&>(
          static_cast<node_element<Tag>&>(*node));
    }

    Container const* operator->() const {
      return &(operator*());
    }

    iterator& operator++() {
      node = node->next();
      return *this;
    }

    iterator operator++(int) {
      iterator copy(*this);
      operator++();
      return copy;
    }

    iterator& operator--() {
      node = node->prev();
      return *this;
    }

    iterator operator--(int) {
      iterator copy(*this);
      operator--();
      return copy;
    }

    bool is_end() const {
      return node->parent == nullptr;
    }

    friend bool operator==(iterator const& a, iterator const& b) {
      return a.node == b.node;
    }

    friend bool operator!=(iterator const& a, iterator const& b) {
      return !(a == b);
    }

    base_node_element* node;
  };

  intrusive_tree(base_node_element* fake, Comparator comparator)
      : tree(comparator), begin_(fake), fake(fake) {}

  ~intrusive_tree() = default;

  iterator insert(Container& element) {
    tree.insert(element);
    update();
    return &element;
  }

  bool erase(Key const& element) {
    auto res = tree.erase(element);
    update();
    return res;
  }

  iterator find(Key const& element) const {
    auto point = tree.find(element);
    return point ? iterator(point) : end();
  }

  iterator lower_bound(Key const& element) const {
    auto point = tree.lower_bound(element);
    return point ? iterator(point) : end();
  }

  iterator upper_bound(Key const& element) const {
    auto point = tree.upper_bound(element);
    return point ? iterator(point) : end();
  }

  iterator begin() const {
    return begin_;
  }

  iterator end() const {
    return fake;
  }

  bool empty() const {
    return tree.empty();
  }

  size_t size() const {
    return tree.size();
  }

  void swap(intrusive_tree& other) {
    tree.swap(other.tree);

    update();
    other.update();
  }

  Comparator const& get_comparator() const {
    return tree.get_comparator();
  }
};

}
