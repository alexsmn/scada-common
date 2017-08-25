#pragma once

#include "base/iterators.h"

template<class Iterator>
class IteratorRange {
 public:
  typedef Iterator iterator;
  typedef Iterator const_iterator;
  typedef typename Iterator::value_type value_type;

  IteratorRange(Iterator begin, Iterator end) : begin_(begin), end_(end) {}

  Iterator begin() const { return begin_; }
  Iterator end() const { return end_; }

  bool empty() const { return begin() == end(); }

  value_type front() const { return *begin(); }
  value_type back() const { return *--end(); }

 private:
  Iterator begin_;
  Iterator end_;
};

/*template<class Pred, class Range>
class FilterRange : public IteratorRange<FilterIterator<Pred, typename Range::const_iterator>> {
 public:
  FilterRange(Pred pred, const Range& range)
      : IteratorRange<FilterIterator<Pred, typename Range::const_iterator>>(
            MakeFilterIterator(pred, std::begin(range), std::end(range)),
            MakeFilterIterator(pred, std::end(range), std::end(range))) {
  }
};*/

template<class Iterator>
inline IteratorRange<Iterator> MakeIteratorRange(Iterator begin, Iterator end) {
  return IteratorRange<Iterator>(begin, end);
}

template<class Pred, class Range>
IteratorRange<FilterIterator<Pred, typename Range::const_iterator>> MakeFilterRange(Pred pred, const Range& range) {
  return MakeIteratorRange(
      MakeFilterIterator(pred, std::begin(range), std::end(range)),
      MakeFilterIterator(pred, std::end(range), std::end(range)));
}

template<class Func, class Range>
IteratorRange<TransformIterator<Func, typename Range::const_iterator>> MakeTransformRange(Func func, const Range& range) {
  return MakeIteratorRange(
      MakeTransformIterator(func, std::begin(range)),
      MakeTransformIterator(func, std::end(range)));
}

template<class Range>
auto MakeReverseRange(Range&& range) -> IteratorRange<decltype(std::rbegin(range))> {
  return MakeIteratorRange(std::rbegin(range), std::rend(range));
}
