#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace client::test {

class TempWorkspace {
 public:
  TempWorkspace();
  ~TempWorkspace();

  const std::filesystem::path& path() const { return path_; }

  void Preserve() { preserve_ = true; }

 private:
  std::filesystem::path path_;
  bool preserve_ = false;
};

void WriteTextFile(const std::filesystem::path& path, std::string_view text);
std::string ReadFileOrEmpty(const std::filesystem::path& path);
bool ContainsInDirectory(const std::filesystem::path& dir,
                         std::string_view needle);
std::optional<int> FindLoggedObjectTreeChildCount(
    const std::filesystem::path& dir);

}  // namespace client::test
