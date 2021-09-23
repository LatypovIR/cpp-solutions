#include "big_integer.h"

#include <cstddef>
#include <cstring>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <algorithm>

uint32_t constexpr BASE = 10;
uint8_t constexpr BITS = 32;

big_integer::big_integer(): negate_(false), data_(1, 0) {}

big_integer::big_integer(big_integer const& other) = default;

big_integer::big_integer(int a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned int a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long a) : big_integer(static_cast<long long>(a)) {}

big_integer::big_integer(unsigned long a) : big_integer(static_cast<unsigned long long>(a)) {}

big_integer::big_integer(long long a) : big_integer(static_cast<unsigned long long>(a)) {
    negate_ = a < 0;
    normalize();
}

big_integer::big_integer(unsigned long long a) : negate_(false) {
    do {
        data_.push_back(static_cast<unsigned int>(a));
    } while ( (a >>= BITS) != 0);
    normalize();
}

big_integer::big_integer(std::string const& str) : big_integer() {

    if (str.empty()) {
        throw std::invalid_argument(str);
    }

    size_t start = (str[0] == '-' || str[0] == '+') ? 1 : 0;
    if (start == str.length()) {
        throw std::invalid_argument(str);
    }
    for (size_t i = start; i < str.length(); i++) {
        if (str[i] < '0' || str[i] > '9') {
            throw std::invalid_argument(str);
        }
    }

    for (size_t i = start; i != str.length(); ++i) {
        (*this) *= BASE;
        (*this) += str[i] - '0';
    }
    if (str[0] == '-') {
        *this = -*this;
    }
}

big_integer::~big_integer() = default;

big_integer& big_integer::operator=(big_integer const& other) = default;

big_integer& big_integer::operator+=(big_integer const& rhs)
{
    add_subtract([](auto a, auto b) { return a + b; }, rhs);
    return *this;
}

big_integer& big_integer::operator-=(big_integer const& rhs)
{
    add_subtract([](auto a, auto b) { return a - b; }, rhs);
    return *this;
}

big_integer& big_integer::operator*=(big_integer const& rhs) {

    if (rhs.negate_) {
        *this = -*this;
        return *this *= -rhs;
    }

    bool sign = negate_;
    if (negate_) {
        *this = -*this;
    }

    std::vector<uint32_t> c(data_.size() + rhs.data_.size() + 1, negate_ ? UINT32_MAX : 0);
    uint64_t carry = 0;
    for (size_t i = 0; i < data_.size(); ++i) {
        for (size_t j = 0; j <= rhs.data_.size(); ++j) {
            c[i + j] = carry = static_cast<uint64_t>(data_[i]) * take_n(j, rhs) + carry + c[i + j];
            carry >>= BITS;
        }
    }
    std::swap(data_, c);
    normalize();

    if (sign) {
        *this = -*this;
    }
    return *this;
}

big_integer& big_integer::operator/=(big_integer const& rhs) {

    if (rhs.negate_) {
        *this = -*this;
        return *this /= -rhs;
    }

    bool sign = negate_;
    if (negate_) {
        *this = -*this;
    }

    divide(rhs);

    normalize();

    if (sign) {
        *this = -*this;
    }

    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    return *this -= ((*this) / rhs) * rhs;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    bit_operator([](auto a, auto b){ return a&b; }, rhs);
    return *this;
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    bit_operator([](auto a, auto b){ return a|b; }, rhs);
    return *this;
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    bit_operator([](auto a, auto b){ return a^b; }, rhs);
    return *this;
}

big_integer& big_integer::operator<<=(int rhs) {
    size_t add = rhs / BITS;
    rhs %= BITS;
    data_.resize(data_.size() + add + 1, negate_ ? UINT32_MAX : 0);
    for (size_t i = data_.size(); i > 0; ) {
        --i;
        if (i < add) {
            data_[i] = 0;
        } else data_[i] = (data_[i - add] << rhs) | ((i == add) ? 0 : (data_[i - add - 1] >> (BITS - rhs)));
    }
    normalize();
    return *this;
}

big_integer& big_integer::operator>>=(int rhs) {
    data_.erase(data_.begin(), data_.begin() + rhs / BITS);
    rhs %= BITS;
    for (size_t i = 0; i < data_.size(); i++)
    {
        data_[i] = (data_[i] >> rhs) |
                   ((i + 1 == data_.size() ? (negate_ ? ((1U << rhs) - 1) : 0U) : (data_[i + 1])) << (BITS - rhs));
    }
    resize(data_.size() + 1);
    normalize();
    return *this;
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    big_integer tmp((*this));
    for (uint32_t& x : tmp.data_) {
        x = ~x;
    }
    tmp.negate_ = !tmp.negate_;
    return ++tmp;
}

big_integer big_integer::operator~() const {
    return --(-(*this));
}

big_integer& big_integer::operator++() {
    return (*this) += 1;
}

big_integer big_integer::operator++(int) {
    big_integer tmp = *this;
    (*this) += 1;
    return tmp;
}

big_integer& big_integer::operator--() {
    return (*this) -= 1;

}

big_integer big_integer::operator--(int) {
    big_integer tmp = *this;
    (*this) -= 1;
    return tmp;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return to_string(a) == to_string(b);
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.negate_ != b.negate_) {
        return a.negate_;
    }
    return (a.compare(b) == -1);
}

bool operator>(big_integer const& a, big_integer const& b) {
    if (a.negate_ != b.negate_) {
        return b.negate_;
    }
    return (a.compare(b) == 1);
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return !(a > b);
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& a) {
    if (a.is_zero()) {
        return "0";
    }
    std::string result;
    big_integer x(a);
    if (x.negate_) x = -x;
    while (!x.is_zero()) {
        result += std::to_string(x.mod());
    }
    if (a.negate_) result += '-';
    std::reverse(result.begin(), result.end());
    return result;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}

bool big_integer::is_zero() const {
    return (!negate_ && data_.size() == 1 && data_.back() == 0);
}


void big_integer::divide(big_integer b) {
    if (data_.size() < b.data_.size()) {
        data_ = std::vector<uint32_t>(1, 0);
        return;
    }

    size_t shift = 1;
    while (b.data_.back() * shift < (1U << (BITS - 1))) {
        shift <<= 1;
    }

    (*this) *= shift;
    b *= shift;

    size_t n = b.data_.size();
    size_t m = data_.size() - b.data_.size();

    std::reverse(b.data_.begin(), b.data_.end());
    b.data_.resize(b.data_.size() + m);
    std::reverse(b.data_.begin(), b.data_.end());

    std::vector<uint32_t> result(m + 1, 0);
    result[m] = 1;

    if ((*this) >= (result[m] * b)) {
        (*this) -= (result[m] * b);
    } else {
        result[m] = 0;
    }

    for (size_t i = m - 1; i < m; --i) {
        b.data_.erase(b.data_.begin());
        result[i] = std::min(
            (((take_n(n + i, *this) * 1ULL) << BITS) | take_n(n + i - 1, *this)) / b.data_.back(),
            (1ULL << BITS) - 1);
        *this -= result[i] * b;
        while (*this < 0) {
            --result[i];
            *this += b;
        }
    }
    data_ = result;
}

void big_integer::normalize() {
    uint32_t nil = negate_ ? UINT32_MAX : 0;
    while (data_.size() > 1 && data_.back() == nil) {
        data_.pop_back();
    }
}

int big_integer::compare(big_integer const& other) const {
    // return
    // -1 abs(this) < abs(other)
    // 0 ==
    // 1 >
    if (data_.size() > other.data_.size()) {
        return 1;
    }
    if (data_.size() < other.data_.size()) {
        return -1;
    }
    for (size_t i = data_.size(); i != 0; ) {
        --i;
        if (data_[i] != other.data_[i]) {
            return (data_[i] < other.data_[i]) ? -1 : 1;
        }
    }
    return 0;
}

uint32_t big_integer::mod()
{
    uint64_t carry = 0;
    for (size_t i = data_.size(); i != 0;)
    {
        --i;
        carry = data_[i] + (carry << BITS);
        data_[i] = carry / BASE;
        carry = carry % BASE;
    }
    normalize();
    return carry;
}

uint32_t big_integer::take_n(size_t i, big_integer const& x) {
    return i < x.data_.size() ? x.data_[i] : (x.negate_ ? UINT32_MAX : 0);
}

void big_integer::resize(size_t new_size) {
    data_.resize(new_size + 2, negate_ ? UINT32_MAX : 0);
}

void big_integer::bit_operator(const std::function<uint32_t (uint32_t, uint32_t)>& f, big_integer const& rhs) {
    resize(std::max(data_.size(), rhs.data_.size()));
    for (size_t i = 0; i < data_.size(); ++i) {
        data_[i] = f(data_[i], take_n(i, rhs));
    }
    negate_ = f(static_cast<uint32_t>(negate_), static_cast<uint32_t>(rhs.negate_)) == 1;
    normalize();
}

void big_integer::add_subtract(const std::function<uint64_t (uint32_t, uint64_t)>& f, big_integer const& rhs) {
    uint64_t carry = 0;
    resize(std::max(data_.size(), rhs.data_.size()));
    for (size_t i = 0; i < data_.size(); ++i) {
        carry = f(data_[i], carry + take_n(i, rhs));
        data_[i] = carry;
        carry = (carry >> BITS) != 0;
    }
    negate_ = (data_.back() >> (BITS - 1)) == 1;
    normalize();
}
