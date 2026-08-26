#include "test/e2e/e2e_file_helpers.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace client::test {
namespace {

int CurrentPid() {
#ifdef _WIN32
  return ::_getpid();
#else
  return static_cast<int>(::getpid());
#endif
}

}  // namespace

void WriteTextFile(const std::filesystem::path& path, std::string_view text) {
  std::error_code ec;
  if (path.has_parent_path())
    std::filesystem::create_directories(path.parent_path(), ec);
  std::ofstream output{path, std::ios::binary | std::ios::trunc};
  if (!output.is_open())
    throw std::runtime_error{"Failed to open " + path.string()};
  output << text;
}

std::string ReadFileOrEmpty(const std::filesystem::path& path) {
  std::ifstream input{path, std::ios::binary};
  if (!input)
    return {};
  return {std::istreambuf_iterator<char>{input},
          std::istreambuf_iterator<char>{}};
}

bool ContainsInDirectory(const std::filesystem::path& dir,
                         std::string_view needle) {
  std::error_code ec;
  if (!std::filesystem::exists(dir, ec))
    return false;

  for (const auto& entry : std::filesystem::directory_iterator{dir, ec}) {
    if (!entry.is_regular_file())
      continue;
    if (ReadFileOrEmpty(entry.path()).find(needle) != std::string::npos)
      return true;
  }
  return false;
}

std::optional<int> FindLoggedObjectTreeChildCount(
    const std::filesystem::path& dir) {
  constexpr std::string_view kCompletionMarker =
      "Children fetched callback completed";
  constexpr std::string_view kAddedChildCountMarker = "AddedChildCount = ";

  std::error_code ec;
  if (!std::filesystem::exists(dir, ec))
    return std::nullopt;

  std::optional<int> result;
  for (const auto& entry : std::filesystem::directory_iterator{dir, ec}) {
    if (!entry.is_regular_file())
      continue;

    const auto contents = ReadFileOrEmpty(entry.path());
    size_t pos = 0;
    while ((pos = contents.find(kCompletionMarker.data(), pos)) !=
           std::string::npos) {
      const auto line_end = contents.find('\n', pos);
      const auto line =
          contents.substr(pos, line_end == std::string::npos ? std::string::npos
                                                             : line_end - pos);
      pos = line_end == std::string::npos ? contents.size() : line_end + 1;

      const auto count_pos = line.find(kAddedChildCountMarker);
      if (count_pos == std::string::npos)
        continue;

      const auto value_begin = count_pos + kAddedChildCountMarker.size();
      size_t value_end = value_begin;
      while (value_end < line.size() && line[value_end] >= '0' &&
             line[value_end] <= '9') {
        ++value_end;
      }
      if (value_end == value_begin)
        continue;

      result = std::stoi(line.substr(value_begin, value_end - value_begin));
      if (*result > 0)
        return result;
    }
  }

  return result;
}

// The workspaces of two runs must never be the same directory: they hold the
// config DB, the license, the logs and the marker files a live process is
// reading and writing. The name used to be a steady_clock tick alone, which
// makes a collision unlikely but neither impossible nor detectable — two
// processes started together can read the same tick, and the create call's
// return value was ignored, so the two runs would silently share one. The PID
// separates concurrent processes (the case that matters: several checkouts
// running their E2Es at once), the tick and counter separate workspaces within
// one, and *checking what the create call returned* — below, taking only a
// candidate `create_directory` reports it actually made — turns the whole
// thing from an expectation into a guarantee. The guarantee comes from reading
// the result, not from the choice of function: `create_directories` reports an
// already-existing leaf through the same `false` return, so the identical bug
// is available under either name.
TempWorkspace::TempWorkspace() {
  const auto base = std::filesystem::temp_directory_path();
  const std::string prefix = "scada_e2e_" + std::to_string(CurrentPid()) + "_";
  for (int attempt = 0; attempt < 100; ++attempt) {
    const auto salt = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    auto candidate = base / (prefix + salt + "_" + std::to_string(attempt));
    std::error_code ec;
    if (std::filesystem::create_directory(candidate, ec)) {
      path_ = std::move(candidate);
      return;
    }
  }
  throw std::runtime_error{"Failed to create a unique E2E workspace under " +
                           base.string()};
}

TempWorkspace::~TempWorkspace() {
  // SCADA_E2E_KEEP_WORKSPACE=1 keeps the tree for post-mortem inspection, the
  // same knob the per-tier startup suites document. Without it a tier that dies
  // during cluster start-up takes its Logs directory with it, and the only
  // evidence left is the harness's "did not start listening" timeout — which
  // says nothing about why.
  if (preserve_ || std::getenv("SCADA_E2E_KEEP_WORKSPACE") != nullptr)
    return;
  std::error_code ec;
  std::filesystem::remove_all(path_, ec);
}

}  // namespace client::test
