#include "base/logger.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "base/file_logger.h"

#ifdef OS_WIN
#include <windows.h>
#endif

std::unique_ptr<Logger> CreateFileLogger(int path_service_key, base::FilePath::StringType base_name, const char* title) {
  base::FilePath path;
  base::PathService::Get(path_service_key, &path);

  auto logger = std::make_unique<FileLogger>();
  logger->Init(std::move(path), std::move(base_name));

  // fill log

  base::FilePath application_path;
  base::PathService::Get(base::FILE_EXE, &application_path);

  logger->Write(LogSeverity::Normal, "======================================================");
  logger->Write(LogSeverity::Normal, title);

  base::Time::Exploded time;
  base::Time::Now().LocalExplode(&time);

  logger->WriteF(LogSeverity::Normal, "Local Time: %02d.%02d.%04d %02d:%02d:%02d",
      time.day_of_month, time.month, time.year, time.hour, time.minute, time.second);

#ifdef OS_WIN
  char comp[MAX_COMPUTERNAME_LENGTH + 1];
  DWORD comp_len = _countof(comp);
  if (!GetComputerNameA(comp, &comp_len))
     strcpy_s(comp, "(Error)");

  logger->WriteF(LogSeverity::Normal, "Computer Name: %s", comp);
#endif

  logger->WriteF(LogSeverity::Normal, "Path: %s", application_path.AsUTF8Unsafe().c_str());
  logger->Write (LogSeverity::Normal, "======================================================");
  
  return std::move(logger);
}
