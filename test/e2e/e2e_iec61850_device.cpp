#include "test/e2e/e2e_iec61850_device.h"

#include "test/e2e/e2e_server_process.h"
#include "test/e2e/e2e_wait.h"

#include <chrono>

using namespace std::chrono_literals;

namespace client::test {

namespace {

// Sized like a tier's start timeout rather than guessed: the device builds a
// small IedModel and binds one socket, so it is up in milliseconds on an idle
// machine, and this is headroom for a loaded one.
constexpr auto kStartTimeout = 5s;

}  // namespace

Iec61850Device::Iec61850Device(const std::filesystem::path& exe,
                               int port,
                               const std::filesystem::path& workdir)
    : port_{port} {
  if (exe.empty() || !std::filesystem::exists(exe)) {
    error_ = "the IEC 61850 device binary is not at " + exe.string();
    return;
  }

  LaunchProcess(exe, {"--port=" + std::to_string(port)}, workdir, job_, child_);

  // Give up the moment the child is gone rather than waiting out the timeout,
  // for the reason ServerTier::WaitListening gives: a device that died has
  // nothing left to wait for, and burning the timeout on a corpse turns a fast,
  // obvious failure into a slow, opaque one.
  WaitUntil([this] { return CanConnectTcp(port_) || !child_.IsRunning(); },
            kStartTimeout);

  started_ = CanConnectTcp(port_);
  if (!started_) {
    error_ = "the IEC 61850 device did not listen on port " +
             std::to_string(port_) + "; see process.stderr.log in " +
             workdir.string();
  }
}

Iec61850Device::~Iec61850Device() {
  // Terminating the group is how every tier is stopped too. The device has no
  // signal handling on purpose, so there is nothing gentler to try first.
  if (started_ || child_.IsRunning())
    ForceTerminate(child_);
  job_.Terminate();
}

bool Iec61850Device::running() const {
  return started_ && child_.IsRunning();
}

}  // namespace client::test
