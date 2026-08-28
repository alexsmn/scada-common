#pragma once

#include "base/lifetime.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace scada {

// A temp directory owned by one test fixture, removed when the fixture is.
//
// **This is the tree's one copy for every product that consumes `common`** —
// the server framework and its tiers, the Qt client, and the Designer. It lived
// in three places until 2026-08-27 (backlog 619), kept in step by hand, which
// is an instruction that works for two copies and stops working at four.
// `third_party/opcuapp` still carries its own: it consumes only `net` and is
// itself consumed by `common`, so it can borrow nothing from here.
// `third_party/sql/sql/test/temp_dir.h` is a fourth of the same shape under
// another name, in a product that likewise does not consume `common`.
//
// Prefer this to naming a directory by hand. The pattern it replaces —
//
//     temp_dir_ = std::filesystem::temp_directory_path() /
//                 ("scada_test_" + std::to_string(
//                      std::chrono::steady_clock::now()
//                          .time_since_epoch().count()));
//     std::filesystem::create_directories(temp_dir_);
//
// — was copy-pasted across the historian's fixtures, then found in seven more
// of the framework's own, then in the client's, the Designer's and opcuapp's,
// and it is wrong in ways that compound. The salt is a clock reading, so
// uniqueness is a probability rather than a fact. The create call's return
// value was *ignored*, and an already-existing directory is reported through
// that return rather than by a failure, so a collision is silent: two fixtures
// open the same SQLite file and the failure surfaces as an unexplained
// assertion somewhere else entirely. And several fixtures shared the same
// prefix, so the clock reading was the only thing keeping them apart from each
// other rather than merely from themselves.
//
// Three further shapes hide the same missing process identity behind something
// that looks deliberate, and all three were found in this tree:
//
//   - A name built from the gtest random seed and the case name, with a
//     `remove_all` before the create to "clean up a leftover". The case name
//     separates cases within a run, but the random seed is 0 unless
//     `--gtest_shuffle` is passed, so two processes running the same case pick
//     the same path — and there the pre-clean is not a tidy-up, it deletes the
//     other run's live directory.
//   - A fixture's own address, `reinterpret_cast<uintptr_t>(this)`. That
//     separates fixtures alive at one moment inside one process and says
//     nothing whatever about a second process.
//   - A fixed name and no salt at all, which is the same bug with the
//     probability set to one.
//
// A salt that is not the PID is not a salt.
//
// Here the PID separates concurrent processes — which is what `ctest -j` and
// two checkouts running tests at once produce — and *checking what the create
// call returned* separates fixtures within one process, by walking the suffix
// until it wins a directory it actually created. None of that is a fact about
// which function to call: for a leaf whose parent exists, `create_directory`
// and `create_directories` behave identically on an already-existing directory
// — both return `false`, and neither throws, in the `error_code` and throwing
// overloads alike (compiled against libc++ and run, 2026-08-26). Reading the
// result is the whole defence, and a reader who takes the lesson to be the
// choice of function will write the same bug with the other name. Uniqueness
// is a guarantee rather than an expectation, and a leftover names the run that
// made it, though there should be none: the destructor removes the tree, which
// the hand-rolled version mostly forgot to do. Mostly, not always — so a
// hand-rolled fixture that does remove its tree is not thereby fine. The
// uniqueness guarantee is the part that is always missing.
//
// Declare it BEFORE any member that opens a file inside it — a
// `sql::connection`, a database handle. Members are destroyed in reverse
// declaration order, so a ScopedTempDir declared last is destroyed first, and
// removing the tree out from under an open handle fails on Windows and silently
// leaves it behind.
//
// `scada_temp_dir_pattern_check` (ctest, at the root) fails on a test that
// calls `std::filesystem::temp_directory_path()` directly, so the pattern
// cannot come back by hand.
class ScopedTempDir {
 public:
  // `prefix` names the fixture in the directory name, for the rare case where
  // one survives a crash and someone has to work out where it came from.
  explicit ScopedTempDir(std::string_view prefix = "scada_test") {
    const std::filesystem::path base = std::filesystem::temp_directory_path();
    const std::string stem =
        std::string{prefix} + "_" + std::to_string(CurrentPid()) + "_";
    for (int attempt = 0; attempt < 1000; ++attempt) {
      std::filesystem::path candidate = base / (stem + std::to_string(attempt));
      std::error_code ec;
      if (std::filesystem::create_directory(candidate, ec)) {
        path_ = std::move(candidate);
        return;
      }
    }
    throw std::runtime_error{"Could not create a unique temp directory under " +
                             base.string()};
  }

  ScopedTempDir(const ScopedTempDir&) = delete;
  ScopedTempDir& operator=(const ScopedTempDir&) = delete;

  ~ScopedTempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path_, ec);
  }

  const std::filesystem::path& path() const SCADA_LIFETIME_BOUND {
    return path_;
  }

 private:
  static int CurrentPid() {
#ifdef _WIN32
    return ::_getpid();
#else
    return static_cast<int>(::getpid());
#endif
  }

  std::filesystem::path path_;
};

}  // namespace scada
