#pragma once
#include <cstddef>


template <class T, size_t N>
class RingBuffer
{
  T data_[N];
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

  const T& operator[] (size_t i) const
  {
    return data_[(begin_ + i) % N];
  }

  size_t size() const
  {
    return size_;
  }

  friend class iterator;

  class iterator
  {
    friend class RingBuffer;

    RingBuffer& rb_;
    size_t offset_;

    iterator(RingBuffer& rb, size_t offset)
        : rb_(rb)
        , offset_(offset)
    {
    }

  public:
    const T& operator* () {
        return rb_[offset_];
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
