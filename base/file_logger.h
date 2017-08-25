#pragma once

#include "base/base_export.h"
#include "base/files/file.h"
#include "base/time/time.h"
#include "base/logger.h"
#include "base/strings/string16.h"

#include <string>

class BASE_EXPORT FileLogger : public Logger {
 public:
  FileLogger();
  virtual ~FileLogger();

  void Init(base::FilePath path, base::FilePath::StringType base_name);
  
  // Logger overrides
  virtual void Write(LogSeverity severity, const char* message) const override;
  virtual void WriteV(LogSeverity severity, const char* message, va_list args) const override;
  virtual void WriteF(LogSeverity severity, const char* format, ...) const override;

  void _Write(const char* data, size_t size) const;

 private:
  base::FilePath::StringType GenerateFileName() const;
 
  void CreateNewFile() const;
  void DeleteOldFiles() const;

  void CloseHandle() const;
 
  base::FilePath path_;
  base::FilePath::StringType base_name_;
  unsigned max_files_;
  int64 max_file_size_;

  mutable base::File file_;
  mutable base::Time::Exploded last_time_;

  DISALLOW_COPY_AND_ASSIGN(FileLogger);
};
