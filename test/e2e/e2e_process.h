#pragma once

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/group.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace client::test {

namespace process = boost::process::v1;

class JobObject {
 public:
  JobObject();
  ~JobObject();

  process::group& group() { return group_; }
  void Terminate();

 private:
  process::group group_;
};

struct ChildProcess {
  mutable process::child process;

  bool IsRunning() const;
  std::optional<int> ExitCode() const;
};

void ForceTerminate(ChildProcess& child);
void WaitForExit(ChildProcess& child, int timeout_ms = 5000);
// Launches `exe` with `args` in `workdir`, tracked by `job`. `extra_env`, when
// non-empty, is layered on top of the current process environment for the child
// only (each pair is a name/value override); an empty list means the child
// inherits the parent environment unchanged.
//
// The child's stdout/stderr are captured to `process.stdout.log` /
// `process.stderr.log` in `capture_dir`, defaulting to `workdir`. Pass an
// explicit `capture_dir` whenever `workdir` is NOT per-run: a server tier runs
// in its own workspace and needs nothing, but the Qt client must run from the
// build output directory (that is where its DLLs and relative paths resolve),
// which every concurrent run of the suite shares. Two runs would then overwrite
// each other's client stderr — the one file that carries a Qt failure the
// client dies too early to log anywhere else.
void LaunchProcess(
    const std::filesystem::path& exe,
    const std::vector<std::string>& args,
    const std::filesystem::path& workdir,
    JobObject& job,
    ChildProcess& child,
    const std::vector<std::pair<std::string, std::string>>& extra_env = {},
    const std::filesystem::path& capture_dir = {});

}  // namespace client::test
