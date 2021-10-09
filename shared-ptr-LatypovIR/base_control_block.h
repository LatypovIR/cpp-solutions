#pragma once

#include <cstddef>
#include <memory>
#include <utility>

namespace control_block {

struct base_control_block {

  explicit base_control_block(size_t n);

  void inc_strong_count();

  void dec_strong_count();

  size_t get_strong_ref_count() const;

  void inc_weak_count();

  void dec_weak_count();

  virtual void delete_object() = 0;

  virtual ~base_control_block() = default;

private:
  size_t strong_ref_count;
  size_t weak_ref_count;
};

}
