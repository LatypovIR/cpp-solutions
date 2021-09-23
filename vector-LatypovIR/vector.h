#pragma once
#include <cstddef>

template <typename T>
struct vector {
  using iterator = T*;
  using const_iterator = T const*;

  vector() : size_(0), capacity_(0), data_(nullptr){};

  vector(vector const& other) : vector() {
    ensure_capacity(other.size_);
    while (size_ < other.size_) {
      save_object(other[size_]);
    }
  };

  vector& operator=(vector const& other) {
    vector(other).swap(*this);
    return *this;
  };

  ~vector() {
    erase_object(data_, size_);
  };

  T& operator[](size_t i) {
    return data_[i];
  };

  T const& operator[](size_t i) const {
    return data_[i];
  };

  T* data() {
    return data_;
  };

  T const* data() const {
    return data_;
  };

  size_t size() const {
    return size_;
  };

  T& front() {
    return data_[0];
  };

  T const& front() const {
    return data_[0];
  };

  T& back() {
    return data_[size_ - 1];
  };

  T const& back() const {
    return data_[size_ - 1];
  };

  void push_back(T const& obj) {
    if (size_ == capacity_) {
      T tmp_obj = obj;
      ensure_capacity(capacity_ == 0 ? 1 : capacity_ * 2);
      save_object(tmp_obj);
    } else {
      save_object(obj);
    }
  };

  void pop_back() {
    data_[--size_].~T();
  };

  bool empty() const {
    return size_ == 0;
  };

  size_t capacity() const {
    return capacity_;
  };

  void reserve(size_t new_capacity_) {
    if (new_capacity_ > capacity_) {
      ensure_capacity(new_capacity_);
    }
  };

  void shrink_to_fit() {
    if (size_ != capacity_) {
      ensure_capacity(size_);
    }
  };

  void clear() {
    while (size_ > 0) {
      pop_back();
    }
  };

  void swap(vector& other) {
    std::swap(other.size_, size_);
    std::swap(other.capacity_, capacity_);
    std::swap(other.data_, data_);
  };

  iterator begin() {
    return data_;
  };

  iterator end() {
    return data_ + size_;
  };

  const_iterator begin() const {
    return data_;
  };

  const_iterator end() const {
    return data_ + size_;
  };

  iterator insert(const_iterator pos, T const& obj) {
    size_t index = pos - begin();
    push_back(obj);
    for (size_t i = size_ - 1; i != index; --i) {
      std::swap(data_[i], data_[i - 1]);
    }
    return begin() + index;
  };

  iterator erase(const_iterator pos) {
    return erase(pos, pos + 1);
  };

  iterator erase(const_iterator first, const_iterator last) {
    if (first >= last)
      return nullptr;
    size_t count = std::distance(first, last),
           start = std::distance(begin(), (iterator)first);
    for (size_t i = start; i + count != size_; i++) {
      std::swap(data_[i], data_[i + count]);
    }
    for (size_t i = 0; i < count; i++) {
      pop_back();
    }
    return data_ + start;
  };

private:
  void ensure_capacity(size_t const new_capacity_) {
    T* new_data_ = new_capacity_ == 0
                     ? nullptr
                     : static_cast<T*>(operator new(new_capacity_ * sizeof(T)));

    for (size_t i = 0; i != size_; ++i) {
      try {
        new (new_data_ + i) T(data_[i]);
      } catch (...) {
        erase_object(new_data_, i);
        throw;
      }
    }
    erase_object(data_, size_);
    data_ = new_data_;
    capacity_ = new_capacity_;
  }

  void save_object(T const& obj) {
    new(data_ + size_) T(obj);
    size_++;
  }

  void erase_object(T* arr, size_t cnt) {
    while (cnt > 0) {
      arr[--cnt].~T();
    }
    operator delete(arr);
  }

  T* data_;
  size_t size_;
  size_t capacity_;
};
