#include "base/thread_executor.h"

bool ThreadExecutor::PendingTask::operator<(const PendingTask& other) const {
  if (time != other.time)
    return time < other.time;
  return sequence - other.sequence < 0;
}

ThreadExecutor::ThreadExecutor()
    : stopped_(false),
      sequence_(0) {
  thread_ = std::thread([this] {
    while (!stopped_) {
      if (auto task = GetTask())
        task();
    }
  });
}

ThreadExecutor::~ThreadExecutor() {
  stopped_ = true;
  condition_.notify_all();
  thread_.join();
}

void ThreadExecutor::PostDelayedTask(Duration delay, Task task) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (delay == Duration()) {
    task_queue_.emplace(std::move(task));
  } else {
    PendingTask pending_task;
    pending_task.task = std::move(task);
    pending_task.time = Clock::now() + delay;
    pending_task.sequence = sequence_++;
    pending_task_queue_.emplace(std::move(pending_task));
  }
  condition_.notify_one();
}

Task ThreadExecutor::GetTask() {
  std::unique_lock<std::mutex> lock(mutex_);

  while (!stopped_) {
    while (!pending_task_queue_.empty() &&
           Clock::now() >= pending_task_queue_.top().time) {
      task_queue_.emplace(std::move(pending_task_queue_.top().task));
      pending_task_queue_.pop();
    }

    if (!task_queue_.empty()) {
      auto task = std::move(task_queue_.front());
      task_queue_.pop();
      return task;
    }

    if (pending_task_queue_.empty()) {
      condition_.wait(lock);
    } else {
      auto delay = pending_task_queue_.top().time - Clock::now();
      condition_.wait_for(lock, delay);
    }
  }

  return nullptr;
}

size_t ThreadExecutor::GetTaskCount() const {
  std::unique_lock<std::mutex> lock(mutex_);
  return pending_task_queue_.size();
}
