#include "test/e2e/e2e_port_reservation.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#include <process.h>
#else
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

namespace client::test {
namespace {

// Where the machine-wide port claims live. One directory shared by every E2E
// process on the host — that is the whole point, so it must not be derived from
// the build tree, the worktree, or anything else that differs between the runs
// being kept apart.
std::filesystem::path PortLockDir() {
  return std::filesystem::temp_directory_path() / "scada-e2e-ports";
}

std::filesystem::path PortLockPath(int port) {
  return PortLockDir() / (std::to_string(port) + ".lock");
}

// Records the owning PID in the lock file so a `lsof`-free post-mortem can say
// which run holds a port. Purely diagnostic; failures are ignored.
#ifdef _WIN32
void WriteOwnerPid(void* handle) {
  const std::string pid = std::to_string(_getpid()) + "\n";
  DWORD written = 0;
  ::SetFilePointer(static_cast<HANDLE>(handle), 0, nullptr, FILE_BEGIN);
  ::WriteFile(static_cast<HANDLE>(handle), pid.data(),
              static_cast<DWORD>(pid.size()), &written, nullptr);
  ::SetEndOfFile(static_cast<HANDLE>(handle));
}
#else
void WriteOwnerPid(int fd) {
  const std::string pid = std::to_string(::getpid()) + "\n";
  if (::ftruncate(fd, 0) != 0)
    return;
  [[maybe_unused]] const auto ignored = ::write(fd, pid.data(), pid.size());
}
#endif

// How many distinct ephemeral ports to probe before giving up. Each iteration
// loses only if another E2E process holds that exact port, so exhausting this
// means dozens of concurrent claims on the same host.
constexpr int kMaxProbes = 64;

}  // namespace

PortReservation::PortReservation(PortReservation&& other) noexcept {
  *this = std::move(other);
}

PortReservation& PortReservation::operator=(PortReservation&& other) noexcept {
  if (this == &other)
    return *this;
  Reset();
  port_ = std::exchange(other.port_, 0);
#ifdef _WIN32
  handle_ = std::exchange(other.handle_, nullptr);
#else
  fd_ = std::exchange(other.fd_, -1);
#endif
  return *this;
}

PortReservation::~PortReservation() {
  Reset();
}

void PortReservation::Reset() {
  port_ = 0;
#ifdef _WIN32
  if (handle_ != nullptr) {
    ::CloseHandle(static_cast<HANDLE>(handle_));
    handle_ = nullptr;
  }
#else
  if (fd_ >= 0) {
    // Closing the descriptor releases the flock; there is nothing else to undo.
    // The lock file itself stays — see the class comment on why unlinking it
    // would break mutual exclusion rather than tidy up after it.
    ::close(fd_);
    fd_ = -1;
  }
#endif
}

PortReservation TryReservePort(int port) {
  PortReservation reservation;
  if (port <= 0 || port > 65535)
    return reservation;

  std::error_code ec;
  std::filesystem::create_directories(PortLockDir(), ec);
  const auto path = PortLockPath(port);

#ifdef _WIN32
  // dwShareMode 0: the handle is exclusive for as long as it is open, and
  // Windows closes it for us if the process dies.
  HANDLE handle =
      ::CreateFileA(path.string().c_str(), GENERIC_WRITE, /*dwShareMode=*/0,
                    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
    return reservation;
  WriteOwnerPid(handle);
  reservation.handle_ = handle;
#else
  const int fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0666);
  if (fd < 0)
    return reservation;
  // LOCK_NB so a contended port fails immediately and the caller probes
  // another, rather than blocking behind whoever is using it.
  if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
    ::close(fd);
    return reservation;
  }
  WriteOwnerPid(fd);
  reservation.fd_ = fd;
#endif

  reservation.port_ = port;
  return reservation;
}

PortReservation ReserveEphemeralPort() {
  boost::asio::io_context io_context;
  // Probes we could not claim. Held open — not closed — for the rest of the
  // call so the OS keeps naming us fresh ports instead of re-offering the ones
  // another process already owns.
  std::vector<boost::asio::ip::tcp::acceptor> rejected;

  for (int probe = 0; probe < kMaxProbes; ++probe) {
    boost::asio::ip::tcp::acceptor acceptor{
        io_context,
        boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 0}};
    const int port = static_cast<int>(acceptor.local_endpoint().port());

    // Claim while still bound: until the lock is ours, another process's
    // bind(0) must not be able to be handed this port either.
    PortReservation reservation = TryReservePort(port);
    if (reservation) {
      // The lock now owns the port; the child process about to be launched
      // needs the socket free to bind it.
      acceptor.close();
      return reservation;
    }

    rejected.push_back(std::move(acceptor));
  }

  throw std::runtime_error{
      "Could not claim a free TCP port after " + std::to_string(kMaxProbes) +
      " attempts; every probed port is held by another E2E process (claims "
      "live in " +
      PortLockDir().string() + ")"};
}

}  // namespace client::test
