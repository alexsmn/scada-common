#pragma once

#include "base/file_path_util.h"
#include "base/files/file_util.h"
#include "base/md5.h"
#include "core/basic_types.h"

#include <filesystem>

const size_t kMaxFileSize = 100 * 1024 * 1024;

inline scada::ByteString CalculateFileHash(const std::filesystem::path& path) {
  std::string contents;
  if (!base::ReadFileToStringWithMaxSize(ToFilePath(path), &contents, kMaxFileSize))
    return {};

  auto md5 = base::MD5String(contents);
  return {md5.begin(), md5.end()};
}
