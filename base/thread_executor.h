#pragma once

#include "base/executor.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

class ThreadExecutor : public Executor {
 public:
  ThreadExecutor();
  virtual ~ThreadExecutor();

  virtual void PostDelayedTask(Duration delay, Task task) override;
  virtual size_t GetTaskCount() const override;

 private:
  Task GetTask();

  struct PendingTask {
    bool operator<(const PendingTask& other) const;

    Task task;
    TimePoint time;
    int sequence;
  };

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::queue<Task> task_queue_;
  std::priority_queue<PendingTask> pending_task_queue_;
  int sequence_;
  std::thread thread_;
  std::atomic<bool> stopped_;
};
