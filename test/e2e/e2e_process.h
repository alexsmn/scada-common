#pragma once

#include <boost/process/v1/child.hpp>
#include <boost/process/v1/group.hpp>

#include <filesystem>
#include <optional>
#include <string>
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
void LaunchProcess(const std::filesystem::path& exe,
                   const std::vector<std::string>& args,
                   const std::filesystem::path& workdir,
                   JobObject& job,
                   ChildProcess& child);

}  // namespace client::test
