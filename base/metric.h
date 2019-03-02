#pragma once

template <class Value>
class Metric {
 public:
  std::size_t count() const { return count_; }
  bool empty() const { return count_ == 0; }

  Value min() const {
    assert(!empty());
    return min_;
  }

  Value max() const {
    assert(!empty());
    return max_;
  }

  Value mean() const {
    assert(!empty());
    return sum_ / count_;
  }

  Value total() const {
    assert(!empty());
    return sum_;
  }

  Metric& operator()(Value value) {
    if (count_ == 0) {
      min_ = max_ = sum_ = value;
      count_ = 1;

    } else {
      if (value < min_)
        min_ = value;
      else if (value > max_)
        max_ = value;
      sum_ += value;
      ++count_;
    }

    return *this;
  }

  void reset() { count_ = 0; }

 private:
  Value max_;
  Value min_;
  Value sum_;
  std::size_t count_ = 0;
};
