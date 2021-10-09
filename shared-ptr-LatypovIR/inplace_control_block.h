#pragma once

#include "base_control_block.h"

namespace control_block {

template <typename T>
struct inplace_control_block : base_control_block {

  template <typename... Args>
  explicit inplace_control_block(Args&&... args) : base_control_block(1) {
    new (&object) T(std::forward<Args>(args)...);
  };

  void delete_object() override {
    get_pointer()->~T();
  }

  T* get_pointer() {
    return &reinterpret_cast<T&>(object);
  }

  ~inplace_control_block() = default;

private:
  std::aligned_storage_t<sizeof(T), alignof(T)> object;
};

}
