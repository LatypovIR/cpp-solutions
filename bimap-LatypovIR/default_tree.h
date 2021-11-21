#pragma once

#include <type_traits>
#include "base_node_element.h"

namespace tree_structs {

template <typename Tag>
struct node_element : base_node_element {};

// ContainerType : node_element<Tag>

// Comparator(Container const&, Container const&) &&
// Comparator(Container const&, T const&) &&
// Comparator(T const&, Container const&)

template <typename Container, typename Key, typename Tag, typename Comparator>
struct default_tree {

private:
  using node = base_node_element;
  size_t size_;
  node* root_;
  Comparator comparator;

  node* as_node(Container* container) const {
    return static_cast<base_node_element*>(
        static_cast<node_element<Tag>*>(container));
  }

  static Container* as_container(node* base_node) {
    return static_cast<Container*>(static_cast<node_element<Tag>*>(base_node));
  }

  node* insert(node* root, Container& element) {
    if (!root) {
      return as_node(&element);
    }

    if (comparator(*as_container(root), element)) {
      root->right = insert(root->right, element);
    } else if (comparator(element, *as_container(root))) {
      root->left = insert(root->left, element);
    } else {
      size_--;
      return root;
    }

    return root->balance();
  }

  static node* find_min(node* root) {
    while (root->left)
      root = root->left;
    return root;
  }

  static node* remove_min(node* root) {
    if (!root->left) {
      return root->right;
    }
    root->left = remove_min(root->left);
    return root->balance();
  }

  node* erase(node* root, Key const& key) {
    if (!root) {
      return nullptr;
    }

    if (comparator(*as_container(root), key)) {
      root->right = erase(root->right, key);
    } else if (comparator(key, *as_container(root))) {
      root->left = erase(root->left, key);
    } else {
      size_--;

      node* l = root->left;
      node* r = root->right;
      if (!r) {
        return l;
      }

      node* min = find_min(r);
      min->right = remove_min(r);
      min->left = l;
      return min->balance();
    }

    return root->balance();
  };

  node* find(node* root, Key const& key) const {
    if (!root) {
      return nullptr;
    }

    if (comparator(*as_container(root), key)) {
      return find(root->right, key);
    } else if (comparator(key, *as_container(root))) {
      return find(root->left, key);
    }
    return root;
  }

public:
  static_assert(std::is_convertible_v<Container&, node_element<Tag>&>,
                "value type is not convertible to base_list_element");

  explicit default_tree(Comparator comparator)
      : size_(0), root_(nullptr), comparator(std::move(comparator)) {}

  ~default_tree() = default;

  Container* insert(Container& element) {
    size_++;
    root_ = insert(root_, element);
    root_->update();
    return &element;
  };

  node* root() const {
    return root_;
  }

  bool erase(Key const& key) {
    size_t old_size_ = size_;
    root_ = erase(root_, key);
    if (root_)
      root_->update();
    return size_ < old_size_;
  }

  Container* find(Key const& key) const {
    return as_container(find(root_, key));
  }

  Container* lower_bound(Key const& key) const {
    node* f = as_node(find(key));
    if (f) {
      return as_container(f);
    }
    return upper_bound(key);
  };

  Container* upper_bound(Key const& key) const {
    node* best = nullptr;
    node* root = root_;

    while (root) {
      if (comparator(key, *as_container(root))) {
        best = root;
        root = root->left;
      } else {
        root = root->right;
      }
    }
    return as_container(best);
  };

  bool empty() const {
    return !root_;
  };

  size_t size() const {
    return size_;
  };

  void swap(default_tree& other) {
    std::swap(size_, other.size_);
    std::swap(root_, other.root_);
  }

  Comparator const& get_comparator() const {
    return comparator;
  }
};
}
