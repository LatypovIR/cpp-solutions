#pragma once

#include "base_control_block.h"

namespace control_block {

template <typename T, typename Deleter = std::default_delete<T>>
struct pointer_control_block : base_control_block {

  explicit pointer_control_block(T* ptr, Deleter&& deleter = Deleter())
      : base_control_block(1), ptr(ptr),
        deleter(std::move(deleter)){};

  explicit pointer_control_block(T* ptr, Deleter const& deleter = Deleter())
      : base_control_block(1), ptr(ptr),
        deleter((deleter)){};

  void delete_object() override {
    deleter(ptr);
    ptr = nullptr;
  }

  ~pointer_control_block() = default;

private:
  T* ptr;
  [[no_unique_address]] Deleter deleter;
};

}
