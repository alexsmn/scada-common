#include "test/e2e/e2e_process.h"

#include <boost/process/v1/args.hpp>
#include <boost/process/v1/io.hpp>
#include <boost/process/v1/start_dir.hpp>

#include <chrono>
#include <stdexcept>
#include <system_error>
#include <thread>

namespace client::test {
namespace {

constexpr int kStillActive = 259;
constexpr auto kWaitStep = std::chrono::milliseconds{50};

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
  if (!child.process.valid() || !child.IsRunning())
    return;

  std::error_code ec;
  child.process.terminate(ec);
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

void LaunchProcess(const std::filesystem::path& exe,
                   const std::vector<std::string>& args,
                   const std::filesystem::path& workdir,
                   JobObject& job,
                   ChildProcess& child) {
  try {
    child.process = process::child{
        exe.string(),
        process::args(args),
        process::start_dir(workdir.string()),
        process::std_out > process::null,
        process::std_err > process::null,
        job.group()};
  } catch (const std::system_error& e) {
    throw std::runtime_error{"Failed to launch " + exe.string() + ": " +
                             e.what()};
  }
}

}  // namespace client::test
