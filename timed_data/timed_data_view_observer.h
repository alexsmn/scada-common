#pragma once

#include "timed_data/timed_data_buffer_fwd.h"

#include <span>

template <typename T>
class BasicTimedDataViewObserver {
 public:
  virtual void OnTimedDataUpdates(std::span<const T> values) {}
  virtual void OnTimedDataReady() {}
};
