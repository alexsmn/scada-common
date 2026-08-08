#pragma once

namespace client::test {

// A loopback TCP port claimed for this process against every other E2E process
// on the machine, for as long as the reservation object lives.
//
// Every suite used to pick its ports by binding port 0, reading the port the OS
// named, and closing the socket again — leaving the port unowned for the
// seconds it takes to copy a fixture, generate a config DB and boot a tier
// before the child finally binds it. Inside one test process a `PortPool`
// covered the gap with a `std::set`, so the race was invisible; across
// processes there was nothing at all. That is fine while only one suite runs at
// a time and false as soon as two do — two checkouts of this tree running their
// E2Es at once (the reason this class exists), or a single `ctest -j` running
// the client suite beside a tier's.
//
// The claim is a lock file under `<temp>/scada-e2e-ports/<port>.lock`, held
// open with an OS-level exclusive lock (`flock` on POSIX, an
// exclusive-share-mode handle on Windows). Both release when the handle closes
// OR when the process dies, so a crashed or killed run never strands a port —
// there is no staleness heuristic to get wrong, and no lock to clean up by
// hand. The files themselves are deliberately never unlinked: deleting one
// while another process holds it open would let two processes lock two
// different inodes for the same port and believe they both won. They are empty
// apart from the owning PID and cost nothing to leave behind.
//
// This excludes other *E2E harness* processes, which is the collision that
// actually happens. It does not stop an unrelated program from binding the
// port — no scheme can, short of the kernel handing us the socket the child
// will bind.
class PortReservation {
 public:
  // An empty reservation, owning nothing; `port()` is 0.
  PortReservation() = default;
  PortReservation(const PortReservation&) = delete;
  PortReservation& operator=(const PortReservation&) = delete;
  PortReservation(PortReservation&& other) noexcept;
  PortReservation& operator=(PortReservation&& other) noexcept;
  ~PortReservation();

  // The claimed port, or 0 when the reservation is empty.
  int port() const { return port_; }

  // True when this reservation actually holds a port.
  explicit operator bool() const { return port_ != 0; }

  // Drops the claim, letting another process take the port.
  void Reset();

 private:
  friend PortReservation TryReservePort(int port);

  int port_ = 0;
#ifdef _WIN32
  // A Win32 HANDLE, kept as void* so this header needs no <Windows.h>.
  void* handle_ = nullptr;
#else
  int fd_ = -1;
#endif
};

// Claims `port` specifically. Returns an empty reservation when another E2E
// process already holds it (or when the lock file cannot be created).
PortReservation TryReservePort(int port);

// Binds an ephemeral loopback port, claims it machine-wide, and returns the
// claim. The probe socket stays bound while the claim is taken, so a port
// another process is about to lose the race for is never handed out twice;
// probes that lose the claim are held open for the rest of the call so the OS
// does not offer the same port again on the next attempt.
//
// Throws std::runtime_error if no port could be claimed, which in practice
// means something is holding a very large number of them.
PortReservation ReserveEphemeralPort();

}  // namespace client::test
