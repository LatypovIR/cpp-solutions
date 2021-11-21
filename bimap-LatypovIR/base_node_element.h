#pragma once

#include <cstddef>

namespace tree_structs {
struct base_node_element {

  base_node_element();
  ~base_node_element() = default;

  void update();

  base_node_element* next();
  base_node_element* prev();
  base_node_element* balance();

  base_node_element* left;
  base_node_element* right;
  base_node_element* parent;
  size_t height;
};
}
