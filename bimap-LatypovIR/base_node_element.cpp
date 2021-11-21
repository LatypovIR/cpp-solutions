#include "base_node_element.h"

namespace tree_structs {

base_node_element::base_node_element()
    : left(nullptr), right(nullptr), parent(nullptr), height(1) {}

static size_t get_height(base_node_element* node) {
  return node ? node->height : 0;
}

static int balance_factor(base_node_element* node) {
  return get_height(node->right) - get_height(node->left);
}

static base_node_element* rotate_right(base_node_element* node) {
  base_node_element* result = node->left;
  node->left = result->right;
  result->right = node;
  node->update();
  result->update();
  return result;
}

static base_node_element* rotate_left(base_node_element* node) {
  base_node_element* result = node->right;
  node->right = result->left;
  result->left = node;
  node->update();
  result->update();
  return result;
}

base_node_element* base_node_element::balance() {

  update();

  if (balance_factor(this) == 2) {
    if (balance_factor(right) < 0) {
      right = rotate_right(right);
    }
      return rotate_left(this);

  }

  if (balance_factor(this) == -2) {
    if (balance_factor(left) > 0) {
      left = rotate_left(left);
    }
      return rotate_right(this);

  }

  return this;
}

static bool left_child(base_node_element* node) {
  return node->parent && node->parent->left == node;
}

static bool right_child(base_node_element* node) {
  return node->parent && node->parent->right == node;
}

void base_node_element::update() {
  if (left)
    left->parent = this;
  if (right)
    right->parent = this;

  size_t a = get_height(left);
  size_t b = get_height(right);
  height = (a > b ? a : b) + 1;
}

base_node_element* base_node_element::next() {
  base_node_element* node = this;

  if (node->right) {
    node = node->right;
    while (node->left) {
      node = node->left;
    }
    return node;
  }
  while (right_child(node)) {
    node = node->parent;
  }
  if (left_child(node)) {
    return node->parent;
  }
  return nullptr;
}

base_node_element* base_node_element::prev() {
  base_node_element* node = this;
  if (node->left) {
    node = node->left;
    while (node->right) {
      node = node->right;
    }
    return node;
  }
  while (left_child(node)) {
    node = node->parent;
  }
  if (right_child(node)) {
    return node->parent;
  }
  return nullptr;
}
}
