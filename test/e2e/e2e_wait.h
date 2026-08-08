#pragma once

#include <chrono>
#include <cstdlib>
#include <thread>

namespace client::test {

// Poll interval used by WaitUntil between predicate checks.
inline constexpr std::chrono::milliseconds kWaitStep{100};

// Multiplies every harness deadline, from SCADA_E2E_TIMEOUT_SCALE. Default 1;
// values below 1 are ignored.
//
// The suites' waits are already sized for "the whole matrix running and many
// tier processes saturating the machine". Running two checkouts' E2Es at once
// doubles that load on the same cores, and a wait that expires only because the
// box is busy fails a test that has nothing wrong with it. This is the knob for
// that: `SCADA_E2E_TIMEOUT_SCALE=2` buys the run twice the patience without
// editing any of the deadlines.
//
// Only ever scales UP, deliberately. Several harness waits must stay strictly
// larger than a deadline inside the *client* binary (it writes its report on
// success or at that deadline, and the harness waits for the report to appear);
// growing the harness side preserves that ordering, shrinking it would invert
// it and turn every slow case into a timeout with no verdict recorded.
//
// Read from the environment on every call rather than cached in a static: it is
// consulted a handful of times per test case, and a getenv is cheaper than the
// lifetime question a cached one would raise.
inline double TimeoutScale() {
  const char* value = std::getenv("SCADA_E2E_TIMEOUT_SCALE");
  if (value == nullptr || *value == '\0')
    return 1.0;
  const double scale = std::strtod(value, nullptr);
  return scale > 1.0 ? scale : 1.0;
}

// A harness deadline in milliseconds, scaled by TimeoutScale(). Use this at
// every WaitUntil call site rather than converting a constant by hand.
template <class Duration>
std::chrono::milliseconds Timeout(Duration timeout) {
  const auto base =
      std::chrono::duration_cast<std::chrono::milliseconds>(timeout);
  return std::chrono::milliseconds{
      static_cast<long long>(base.count() * TimeoutScale())};
}

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
