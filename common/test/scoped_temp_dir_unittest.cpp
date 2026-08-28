#include "common/test/scoped_temp_dir.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace scada {
namespace {

// Two fixtures alive at once must not share a directory. This is the case the
// hand-rolled pattern actually got wrong: four historian fixtures derived their
// name from the same "scada_test_" prefix plus a clock reading, and
// create_directories() succeeds on a directory that already exists — so a
// collision handed two fixtures one SQLite file with no error anywhere.
//
// These cases used to exist twice, once in the framework's tree and once in the
// client's, because the class did (backlog 619). One class, one set of cases.
TEST(ScopedTempDirTest, ConcurrentInstancesGetDistinctDirectories) {
  std::set<std::filesystem::path> paths;
  // unique_ptr because ScopedTempDir owns a directory and is deliberately
  // neither copyable nor movable.
  std::vector<std::unique_ptr<ScopedTempDir>> dirs;
  for (int i = 0; i < 8; ++i) {
    dirs.push_back(std::make_unique<ScopedTempDir>("scoped_temp_dir_test"));
    EXPECT_TRUE(paths.insert(dirs.back()->path()).second)
        << "handed out twice: " << dirs.back()->path();
    EXPECT_TRUE(std::filesystem::exists(dirs.back()->path()));
  }
}

// Same prefix, same process, still distinct — the suffix has to keep walking
// rather than assume the first candidate is free.
TEST(ScopedTempDirTest, SamePrefixStillYieldsDistinctDirectories) {
  const ScopedTempDir first{"scoped_temp_dir_shared_prefix"};
  const ScopedTempDir second{"scoped_temp_dir_shared_prefix"};
  EXPECT_NE(first.path(), second.path());
}

TEST(ScopedTempDirTest, RemovesTheTreeOnDestruction) {
  std::filesystem::path path;
  {
    const ScopedTempDir dir{"scoped_temp_dir_test"};
    path = dir.path();
    ASSERT_TRUE(std::filesystem::exists(path));
    // Contents too, not just an empty directory: these fixtures leave SQLite
    // databases behind, and the pattern this replaces leaked all of them.
    std::ofstream{path / "history"} << "payload";
    std::filesystem::create_directories(path / "nested" / "deeper");
    ASSERT_TRUE(std::filesystem::exists(path / "nested" / "deeper"));
  }
  EXPECT_FALSE(std::filesystem::exists(path));
}

// A directory that outlives its process — a crash, a SIGKILL — has to say which
// run made it, so it can be attributed rather than guessed at.
TEST(ScopedTempDirTest, NameCarriesThePrefixAndThePid) {
  const ScopedTempDir dir{"scoped_temp_dir_test"};
  const std::string name = dir.path().filename().string();
  EXPECT_TRUE(name.starts_with("scoped_temp_dir_test_")) << name;
#ifdef _WIN32
  const std::string pid = std::to_string(::_getpid());
#else
  const std::string pid = std::to_string(::getpid());
#endif
  EXPECT_NE(name.find("_" + pid + "_"), std::string::npos)
      << name << " does not carry pid " << pid;
}

TEST(ScopedTempDirTest, LivesUnderTheSystemTempDirectory) {
  const ScopedTempDir dir{"scoped_temp_dir_test"};
  // equivalent() rather than ==: temp_directory_path() carries a trailing
  // separator on macOS, which parent_path() does not, so the two compare
  // unequal as strings while naming the same directory.
  EXPECT_TRUE(std::filesystem::equivalent(
      dir.path().parent_path(), std::filesystem::temp_directory_path()));
}

}  // namespace
}  // namespace scada
