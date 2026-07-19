#include "test/e2e/e2e_process.h"

#include <boost/process/v1/args.hpp>
#include <boost/process/v1/env.hpp>
#include <boost/process/v1/environment.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/start_dir.hpp>

#include <chrono>
#include <stdexcept>
#include <system_error>
#include <thread>

#ifndef _WIN32
#include <csignal>
#include <sys/types.h>
#endif

namespace client::test {
namespace {

constexpr int kStillActive = 259;
constexpr auto kWaitStep = std::chrono::milliseconds{50};

// Hard-kills a child by its native pid. boost::process::v1::terminate can no-op
// on macOS when its internal waitpid has already lost/reaped the child (e.g. the
// process left the launcher's tracking), leaving the process alive — every
// cluster tier then survives until the test binary exits and its whole set
// accumulates across a sequential run. A direct SIGKILL to the pid the launcher
// recorded is reliable regardless of that tracking state.
void HardKill(process::child& process) {
#ifndef _WIN32
  const auto pid = process.id();
  if (pid > 0)
    ::kill(static_cast<pid_t>(pid), SIGKILL);
#else
  (void)process;
#endif
}

}  // namespace

JobObject::JobObject() = default;

JobObject::~JobObject() {
  Terminate();
}

void JobObject::Terminate() {
  if (!group_.valid())
    return;

  std::error_code ec;
  group_.terminate(ec);
}

bool ChildProcess::IsRunning() const {
  if (!process.valid())
    return false;

  std::error_code ec;
  return process.running(ec) && !ec;
}

std::optional<int> ChildProcess::ExitCode() const {
  if (!process.valid())
    return std::nullopt;
  if (IsRunning())
    return kStillActive;
  return process.exit_code();
}

void ForceTerminate(ChildProcess& child) {
  if (!child.process.valid())
    return;

  // Do NOT gate on IsRunning(): on macOS boost's running()/waitpid can report a
  // still-live child as not-running (its tracking lost the pid), which used to
  // skip termination and leak the process. Terminate via boost, then SIGKILL the
  // recorded pid directly as a reliable fallback.
  std::error_code ec;
  child.process.terminate(ec);
  HardKill(child.process);
  WaitForExit(child);
}

void WaitForExit(ChildProcess& child, int timeout_ms) {
  if (!child.process.valid())
    return;

  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds{timeout_ms};
  while (std::chrono::steady_clock::now() < deadline) {
    if (!child.IsRunning()) {
      std::error_code ec;
      child.process.wait(ec);
      return;
    }
    std::this_thread::sleep_for(kWaitStep);
  }
}

void LaunchProcess(
    const std::filesystem::path& exe,
    const std::vector<std::string>& args,
    const std::filesystem::path& workdir,
    JobObject& job,
    ChildProcess& child,
    const std::vector<std::pair<std::string, std::string>>& extra_env) {
  try {
    if (extra_env.empty()) {
      child.process = process::child{
          exe.string(),
          process::args(args),
          process::start_dir(workdir.string()),
          process::std_out > process::null,
          process::std_err > process::null,
          job.group()};
    } else {
      process::environment env = boost::this_process::environment();
      for (const auto& [name, value] : extra_env)
        env[name] = value;
      child.process = process::child{
          exe.string(),
          process::args(args),
          process::start_dir(workdir.string()),
          process::std_out > process::null,
          process::std_err > process::null,
          env,
          job.group()};
    }
  } catch (const std::system_error& e) {
    throw std::runtime_error{"Failed to launch " + exe.string() + ": " +
                             e.what()};
  }
}

}  // namespace client::test
