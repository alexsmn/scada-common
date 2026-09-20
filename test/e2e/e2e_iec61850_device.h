#pragma once

#include "test/e2e/e2e_process.h"

#include <filesystem>
#include <string>

namespace client::test {

// The simulated IEC 61850 IED, as a child process (backlog 801).
//
// It was an in-process object until 2026-09-20, which meant every suite that
// linked the harness compiled libiec61850 -- and so acquired the unpublished
// `iec61850pp` product and a git submodule -- for the sake of a fake device.
// The eight processes in an E2E topology are now handed over the same way:
// seven tier binaries and this one, each named by path (ADR 0011).
//
// Absence is a supported state, exactly as for a tier binary: a build that was
// given no device path constructs none, and the parameters that need one skip.
class Iec61850Device {
 public:
  // Launches `exe` with `--port=<port>` in `workdir` and waits until it accepts
  // a TCP connection there, or until it exits. Never throws: a device that will
  // not start leaves `running()` false and `error()` set, which is what a
  // fixture asserts on.
  Iec61850Device(const std::filesystem::path& exe,
                 int port,
                 const std::filesystem::path& workdir);

  // Terminates the process group. The device has no quit protocol and needs
  // none -- see the binary's own main() for why.
  ~Iec61850Device();

  Iec61850Device(const Iec61850Device&) = delete;
  Iec61850Device& operator=(const Iec61850Device&) = delete;

  // True once the device accepted a connection on its port and the process is
  // still alive.
  bool running() const;

  // Why it did not come up, in a form a fixture can print. Empty while running.
  // The process's own stderr is captured to `process.stderr.log` in `workdir`.
  const std::string& error() const { return error_; }

 private:
  const int port_;
  JobObject job_;
  ChildProcess child_;
  bool started_ = false;
  std::string error_;
};

}  // namespace client::test
