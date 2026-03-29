#pragma once

#include "base/md5.h"
#include "scada/basic_types.h"

#include <filesystem>
#include <fstream>
#include <string>

const size_t kMaxFileSize = 100 * 1024 * 1024;

inline scada::ByteString CalculateFileHash(const std::filesystem::path& path) {
  std::string contents;

  std::ifstream ifs{path, std::ios::binary};
  if (!ifs)
    return {};
  ifs.seekg(0, std::ios::end);
  auto size = ifs.tellg();
  if (size > static_cast<std::streamoff>(kMaxFileSize))
    return {};
  ifs.seekg(0, std::ios::beg);
  contents.resize(static_cast<size_t>(size));
  ifs.read(contents.data(), size);

  auto md5 = base::MD5String(contents);
  return {md5.begin(), md5.end()};
}
