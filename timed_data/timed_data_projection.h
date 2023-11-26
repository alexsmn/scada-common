#pragma once

#include "timed_data_view.h"

template <typename T>
class BasicTimedDataProjection {
 public:
  explicit BasicTimedDataProjection(const BasicTimedDataView<T>& view)
      : view_{view} {}

  void SetTimeRange(const scada::DateTimeRange& time_range) {
    time_range_ = time_range;

    first_index_ = 0;
    count_ = 0;
  }

  int size() const { return count_; }
  const T& at(int index) const { return view_.at(first_index_ + index); }

 private:
  const BasicTimedDataView<T>& view_;

  scada::DateTimeRange time_range_;

  int first_index_ = 0;
  int count_ = 0;
};
