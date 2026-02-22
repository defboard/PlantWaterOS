#pragma once
#include <cstddef>
#include <iterator>


template <class T, size_t N>
class RingBuffer
{
  T data_[N] { };
  size_t size_ = 0;
  size_t begin_ = 0;

public:
  void push_back(T value)
  {
    data_[(begin_ + size_) % N] = value;

    if (size_ < N) {
      ++size_;
    }
    else {
      begin_ = (begin_ + 1) % N;
    }
  }

  const T& back() const
  {
    return at(size_ - 1);
  }

  const T& at_pos(size_t i) const
  {
    return data_[i % N];
  }

  const T& at(size_t i) const
  {
    return data_[(begin_ + i) % N];
  }

  const T& operator[] (size_t i) const
  {
    return at(i);
  }

  size_t size() const
  {
    return size_;
  }

  static constexpr size_t capacity()
  {
    return N;
  }

  size_t pos() const
  {
    return (begin_ + size_) % N;
  }

  class iterator
  {
    friend class RingBuffer;

    const RingBuffer& rb_;
    size_t offset_;

    iterator(const RingBuffer& rb, size_t offset)
        : rb_(rb)
        , offset_(offset)
    {
    }

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = long;
    using pointer = const T*;
    using reference = const T&;

    reference operator* () {
        return rb_[offset_];
    }

    pointer operator-> () {
        return &rb_[offset_];
    }

    iterator& operator++() {
        ++offset_;
        return *this;
    }

    iterator operator++(int) {
        return iterator(rb_, offset_++);
    }

    bool operator == (const iterator& other) const {
        return offset_ == other.offset_;
    }

    bool operator != (const iterator& other) const {
        return not (*this == other);
    }

    long operator - (const iterator& other) const {
        return offset_ - other.offset_;
    }

    iterator operator + (size_t num) {
        num = std::min(num, rb_.size() - offset_);
        return iterator(rb_, offset_ + num);
    }
    iterator operator - (size_t num) {
        num = std::min(num, offset_);
        return iterator(rb_, offset_ - num);
    }
  };

  iterator begin() const
  {
    return iterator(*this, 0);
  }

  iterator end() const
  {
    return iterator(*this, size_);
  }

};
