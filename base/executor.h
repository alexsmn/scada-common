#pragma once

#include "base/common_types.h"

class Executor {
 public:
  virtual ~Executor() {}

  void PostTask(Task task) {
    PostDelayedTask(Duration(), std::move(task));
  }

  virtual void PostDelayedTask(Duration delay, Task task) = 0;

  virtual size_t GetTaskCount() const = 0;
};
