#pragma once

template <class T>
class span {
 public:
  span(T* begin, T* end) : begin_{begin}, end_{end} {}

  size_t size() const { return end_ - begin_; }
  bool empty() const { return begin_ == end_; }

  T& front() const { return *begin_; }
  T& back() const { return *(end_ - 1); }

  T* begin() const { return begin_; }
  T* end() const { return end_; }

 private:
  T* begin_;
  T* end_;
};
