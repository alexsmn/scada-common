#pragma once

#include <chrono>
#include <thread>

namespace client::test {

// Poll interval used by WaitUntil between predicate checks.
inline constexpr std::chrono::milliseconds kWaitStep{100};

// Polls `predicate` until it returns true or `timeout` elapses, sleeping
// `step` between checks. Returns the final predicate value, so a timeout still
// reports the last observation. Shared by the E2E harness (the cluster, the
// server-process wrapper) and the client E2E support TU; keep it here rather
// than copied per translation unit.
template <class Predicate>
bool WaitUntil(Predicate&& predicate,
               std::chrono::milliseconds timeout,
               std::chrono::milliseconds step = kWaitStep) {
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  while (std::chrono::steady_clock::now() < deadline) {
    if (predicate())
      return true;
    std::this_thread::sleep_for(step);
  }
  return predicate();
}

}  // namespace client::test
