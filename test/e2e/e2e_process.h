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
void LaunchProcess(
    const std::filesystem::path& exe,
    const std::vector<std::string>& args,
    const std::filesystem::path& workdir,
    JobObject& job,
    ChildProcess& child,
    const std::vector<std::pair<std::string, std::string>>& extra_env = {});

}  // namespace client::test
