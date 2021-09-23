//
// Created by Ildar on 01.06.2021.
//

#include <memory>

template<typename T, size_t SMALL_SIZE>
struct socow_vector {

    using iterator = T *;
    using const_iterator = T const *;

    socow_vector() : size_(0) {};

    socow_vector(socow_vector const &other) : size_(other.size() << 1) {
        // WARNING : other.size_ is bad, now we small vector
        if (other.small()) {
            copy_array(data(), other.data(), size());
        } else {
            toBigType(other.big_data);
        }
    };

    T *data() {
        make_copy();
        return const_data();
    }

    T const *data() const {
        return const_data();
    };

    socow_vector &operator=(socow_vector const &other) {
        socow_vector(other).swap(*this);
        return *this;
    };

    ~socow_vector() {
        if (unique()) {
            clear_array(data(), size());
        }
        toSmallType();
    };

    T const &operator[](size_t i) const {
        return data()[i];
    };

    T &operator[](size_t i) {
        make_copy();
        return data()[i];
    };

    size_t size() const {
        return size_ >> 1;
    };

    T &front() {
        make_copy();
        return data()[0];
    };

    T const &front() const {
        return data()[0];
    };

    T &back() {
        make_copy();
        return data()[size() - 1];
    };

    T const &back() const {
        return data()[size() - 1];
    };

    void push_back(T const &obj) {
        if (size() == capacity()) {
            T tmp_obj = obj;
            ensure_capacity(capacity() == 0 ? 1 : capacity() * 2);
            new(data() + size()) T(tmp_obj);
        } else {
            new(data() + size()) T(obj);
        }
        size_ += 2; // last bit is small
    };

    void pop_back() {
        make_copy();
        data()[size() - 1].~T();
        size_ -= 2;
    };

    bool empty() const {
        return size() == 0;
    };

    size_t capacity() const {
        return small() ? SMALL_SIZE : big_data->capacity;
    };

    void reserve(size_t new_capacity_) {
        make_copy();
        if (new_capacity_ <= capacity()) {
            return;
        }
        ensure_capacity(new_capacity_);
    };

    void shrink_to_fit() {
        if (small() || size() == capacity()) {
            return;
        }
        make_copy();

        if (size() > SMALL_SIZE) {
            ensure_capacity(size());
            return;
        }
        //Big to small
        dynamic_storage* copy = big_data;
        ++(copy->counter);
        toSmallType();
        try {
            copy_array(const_data(), copy->array, size());
        } catch (...) {
            --(copy->counter);
            toBigType(copy);
            throw;
        }

        if (--(copy->counter) == 0) {
            clear_array(copy->array, size());
            operator delete(copy);
        }

    };

    void clear() {
        make_copy();
        clear_array(data(), size());
        size_ = size_ & 1;
    };

    void swap(socow_vector &other) {
        if (small() && other.small()) {
            if (size() > other.size()) {
                other.swap(*this);
                return;
            }
            for (size_t i = 0; i < size(); i++) {
                if (i < size()) {
                    std::swap((*this)[i], other[i]);
                }
            }
            copy_array(data() + size(), other.data() + size(), other.size() - size());
            clear_array(other.data() + size(), other.size() - size());
            std::swap(size_, other.size_);

        } else if (!small() && !other.small()) {
            std::swap(size_, other.size_);
            std::swap(big_data, other.big_data);

        } else if (small() && !other.small()) {
            reserve(SMALL_SIZE + 1);
            swap(other);
            other.shrink_to_fit();
        } else {
            other.swap(*this);
        }
    };

    iterator begin() {
        make_copy();
        return data();
    };

    iterator end() {
        make_copy();
        return data() + size();
    };

    const_iterator begin() const {
        return data();
    };

    const_iterator end() const {
        return data() + size();
    };

    iterator insert(const_iterator pos, T const &obj) {
        size_t index = pos - const_data();
        push_back(obj); // auto update
        for (size_t i = size() - 1; i != index; --i) {
            std::swap(data()[i], data()[i - 1]);
        }
        return begin() + index;
    };

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    };

    iterator erase(const_iterator first, const_iterator last) {
        size_t count = std::distance(first, last),
                start = std::distance(const_data(), const_cast<iterator>(first));

        make_copy();

        for (size_t i = start; i + count != size(); i++) {
            std::swap(data()[i], data()[i + count]);
        }

        for (size_t i = 0; i < count; i++) {
            pop_back();
        }

        return begin() + start;
    };

private:
    void ensure_capacity(size_t new_capacity_) {
        if (small() && new_capacity_ <= SMALL_SIZE) {
            return;
        }
        // size_t capacity + T* array

        dynamic_storage *new_storage =
                reinterpret_cast<dynamic_storage*>(operator new(new_capacity_ * sizeof(T) + sizeof(dynamic_storage)));
        new_storage->capacity = new_capacity_;
        new_storage->counter = 0;

        try {
            copy_array(new_storage->array, const_data(), size());
        } catch (...) {
            operator delete(new_storage);
            throw;
        }

        if (unique()) {
            clear_array(data(), size());
        }
        toBigType(new_storage);
    };

    void clear_array(T *arr, size_t cnt) {
        while (cnt > 0) {
            arr[--cnt].~T();
        }
    }

    void copy_array(T *result, T const *arr, size_t cnt) {
        for (size_t i = 0; i != cnt; ++i) {
            try {
                new(result + i) T(arr[i]);
            } catch (...) {
                clear_array(result, i);
                throw;
            }
        }
    }

    void make_copy() {
        if (unique()) {
            return;
        }
        ensure_capacity(capacity());
    }

    struct dynamic_storage {
        size_t counter;
        size_t capacity;
        T array[];
    };

    union {
        T small_data[SMALL_SIZE];
        dynamic_storage* big_data;
    };

    size_t size_;

    bool small() const {
        return (size_ & 1) == 0;
    }

    void toSmallType() {
        if (!small()) {
            if (--(big_data->counter) == 0) {
                operator delete(big_data);
            }
            size_ = (size_ >> 1) << 1;
        }
    }

    void toBigType(dynamic_storage* other) {
        if (!small()) {
            if (--(big_data->counter) == 0) {
                operator delete(big_data);
            }
        }
        big_data = other;
        ++(big_data->counter);
        size_ = size_ | 1;
    }

    T *const_data() const {
        return small() ? const_cast<T*>(small_data) : big_data->array;
    }

    bool unique() const {
        return small() || big_data->counter == 1;
    }
};
