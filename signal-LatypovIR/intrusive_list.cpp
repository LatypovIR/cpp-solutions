#include "intrusive_list.h"

  intrusive::base_list_element::base_list_element() {
    prev = next = nullptr;
  }

  intrusive::base_list_element::base_list_element(base_list_element &&other) {
    *this = std::move(other);
  }

  intrusive::base_list_element& intrusive::base_list_element::operator=(base_list_element &&other) {
    if(&other == this) {
      return *this;
    }

    unlink();

    prev = other.prev;
    next = other.next;

    other.prev->next = this;
    other.next->prev = this;

    other.prev = other.next = &other;
    return *this;
  }

  intrusive::base_list_element::~base_list_element() {
    unlink();
  }

  void intrusive::base_list_element::unlink() {
    if (prev == this || prev == nullptr) {
      return;
    }
    prev->next = next;
    next->prev = prev;
    next = this;
    prev = this;
  }
  void intrusive::base_list_element::insert(base_list_element* other) {
    other->unlink();
    other->prev = prev;
    prev->next = other;
    other->next = this;
    prev = other;
  }
  void
  intrusive::base_list_element::splice(intrusive::base_list_element* first,
                                       intrusive::base_list_element* last) {
    if (first == last) {
      return;
    }
    auto triswap = [](base_list_element*& a, base_list_element*& b,
                      base_list_element*& c) {
      base_list_element* tmp = a;
      a = b;
      b = c;
      c = tmp;
    };

    triswap(prev->next, first->prev->next, last->prev->next);
    triswap(prev, last->prev, first->prev);
  }
