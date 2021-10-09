#pragma once

#include "base_control_block.h"

namespace control_block {

  base_control_block::base_control_block(size_t n)
      : strong_ref_count(n), weak_ref_count(n) {}

  void base_control_block::inc_strong_count() {
    strong_ref_count++;
    inc_weak_count();
  }

  void base_control_block::dec_strong_count() {
    strong_ref_count--;
    if (strong_ref_count == 0) {
      delete_object();
    }
    dec_weak_count();
  }

  size_t base_control_block::get_strong_ref_count() const {
    return strong_ref_count;
  }

  void base_control_block::inc_weak_count() {
    weak_ref_count++;
  }

  void base_control_block::dec_weak_count() {
    weak_ref_count--;
    if (weak_ref_count == 0) {
      delete this;
    }
  }

}
