#include "test/e2e/e2e_file_helpers.h"

#include <chrono>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace client::test {

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
      const auto line = contents.substr(
          pos,
          line_end == std::string::npos ? std::string::npos : line_end - pos);
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

TempWorkspace::TempWorkspace() {
  auto base = std::filesystem::temp_directory_path();
  auto salt =
      std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
  path_ = base / ("scada_e2e_" + salt);
  std::filesystem::create_directories(path_);
}

TempWorkspace::~TempWorkspace() {
  if (preserve_)
    return;
  std::error_code ec;
  std::filesystem::remove_all(path_, ec);
}

}  // namespace client::test
