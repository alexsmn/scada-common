#pragma once

template<class Pred, class Iterator>
class FilterIterator : public std::iterator<typename Iterator::iterator_category,
                                            typename Iterator::value_type> {
 public:
  using Supertype = std::iterator<typename Iterator::iterator_category,
                                  typename Iterator::value_type>;

  FilterIterator(Pred pred, Iterator it, Iterator end)
    : pred_(pred),
      it_(it),
      end_(end) {
    while (it_ != end_ && !pred_(*it_))
      ++it_;
  }

  FilterIterator& operator++() {
    ++it_;
    while (it_ != end_ && !pred_(*it_))
      ++it_;
    return *this;
  }

  FilterIterator& operator--() {
    --it_;
    while (!pred_(*it_))
      --it_;
    return *this;
  }

  bool operator==(const FilterIterator& other) const { return it_ == other.it_;  }
  bool operator!=(const FilterIterator& other) const { return !operator==(other); }
  bool operator<(const FilterIterator& other) const { return it_ < other.it_; }

  typename Supertype::value_type operator*() const { return *it_; }

 private:
  Pred pred_;
  Iterator it_;
  Iterator end_;
};

template<class Pred, class Iterator>
FilterIterator<Pred, Iterator> MakeFilterIterator(Pred pred, Iterator it, Iterator end) {
  return FilterIterator<Pred, Iterator>(pred, it, end);
}

template<class Func, class Iterator>
class TransformIterator : public std::iterator<typename Iterator::iterator_category,
                                               typename std::result_of<Func(typename Iterator::value_type)>::type> {
 public:
  using Supertype = std::iterator<typename Iterator::iterator_category,
                                  typename std::result_of<Func(typename Iterator::value_type)>::type>;

  TransformIterator(Func func, Iterator it) : func_(func), it_(it) {}

  TransformIterator& operator++() { ++it_; return *this; }
  TransformIterator& operator--() { --it_; return *this; }

  bool operator==(const TransformIterator& other) const { return it_ == other.it_;  }
  bool operator!=(const TransformIterator& other) const { return !operator==(other); }
  bool operator<(const TransformIterator& other) const { return it_ < other.it_; }

  typename Supertype::value_type operator*() const { return func_(*it_); }

 private:
  Func func_;
  Iterator it_;
};

template<class Func, class Iterator>
TransformIterator<Func, Iterator> MakeTransformIterator(Func func, Iterator iterator) {
  return TransformIterator<Func, Iterator>(func, iterator);
}
