#pragma once

#include <exception>

struct bad_variant_access : std::exception {
  const char * what() const noexcept override {
    return "bad variant access.";
  }
};

namespace details {
struct unreachable_throw : std::exception {
  const char* what() const noexcept override {
    return "unreachable_throw in lambda_wrapper.";
  }
};
}
