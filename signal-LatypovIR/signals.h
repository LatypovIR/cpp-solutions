#pragma once
#include <functional>
#include "intrusive_list.h"

// Чтобы не было коллизий с UNIX-сигналами реализация вынесена в неймспейс, по
// той же причине изменено и название файла
namespace signals {

template <typename T>
struct signal;

template <typename... Args>
struct signal<void(Args...)> {
  using callback_t = std::function<void(Args...)>;

  struct conection_tag;

  struct connection : intrusive::list_element<conection_tag> {

    connection() : sig(nullptr), cb() {}

    connection(signal* sig, callback_t cb) : sig(sig), cb(std::move(cb)) {}

    connection(connection&& other) : connection() {
      *this = std::move(other);
    }

    connection& operator=(connection&& other) {
      disconnect();
      cb = std::move(other.cb);
      sig = other.sig;
      if (sig) {
        auto it = sig->callbacks.as_iterator(&other);
        auto this_it = sig->callbacks.insert(it, *this);

        for (auto* cur = sig->tail; cur != nullptr; cur = cur->next) {
          if (cur->it == it) {
            cur->it = this_it;
          }
        }

        sig->callbacks.erase(it);
      }
      return *this;
    }

    void disconnect() noexcept {
      if (!sig) {
        return;
      }
      for (auto cur = sig->tail; cur != nullptr; cur = cur->next) {
        if (&*cur->it == this) {
          ++cur->it;
        }
      }
      this->unlink();
      sig = nullptr;
      cb = {};
    }

    ~connection() {
      disconnect();
    }

    callback_t cb;
    signal* sig;
    friend struct signal;
  };

  signal() : tail(nullptr) {};

  signal(signal const&) = delete;
  signal& operator=(signal const&) = delete;

  signal(signal&& other) : callbacks(std::move(other.callbacks)), tail(other.tail) {
    for (auto it = callbacks.begin(); it != callbacks.end(); ++it) {
      it->sig = this;
    }
  }

  connection connect(callback_t slot) noexcept {
    connection c(this, std::move(slot));
    callbacks.push_back(c);
    return c;
  }

  struct iteration_token {
    explicit iteration_token(signal const* sig) :
          next(sig->tail), sig(sig), it(sig->callbacks.begin()) {
      sig->tail = this;
    }

    ~iteration_token() {
      if (sig) {
        sig->tail = next;
      }
    }

    typename intrusive::list<connection, conection_tag>::const_iterator it;
    iteration_token* next = nullptr;
    signal const* sig = nullptr;
  };

  ~signal() {
    while (tail) {
      tail->sig = nullptr;
      tail = tail->next;
    }

    while (!callbacks.empty()) {
      auto& con = callbacks.back();
      con.unlink();
      con.sig = nullptr;
      con.cb = {};
    }
  }

  void operator()(Args... args) const {
    iteration_token token(this);

    auto end = callbacks.end();
    while (token.it != end && token.sig) {
      auto cur = token.it;
      ++token.it;
      cur->cb(args...);
    }
  }

private:
  intrusive::list<connection, conection_tag> callbacks;
  mutable iteration_token* tail;
};

} // namespace signals
