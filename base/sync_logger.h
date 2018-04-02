#pragma once

#include "base/logger.h"
#include "base/strings/stringprintf.h"

#include <boost/asio/io_context.hpp>

class SyncLogger final : public Logger {
 public:
  SyncLogger(std::shared_ptr<const Logger> logger,
             boost::asio::io_context& io_context)
      : logger_{std::move(logger)}, io_context_{io_context} {}

  virtual void Write(LogSeverity severity, const char* message) const override {
    io_context_.dispatch(
        [logger = logger_, severity, copied_message = std::string{message}] {
          logger->Write(severity, copied_message.c_str());
        });
  }

  virtual void WriteV(LogSeverity severity,
                      const char* format,
                      va_list args) const override {
    io_context_.dispatch(
        [logger = logger_, severity,
         formatted_message = base::StringPrintV(format, args)] {
          logger->Write(severity, formatted_message.c_str());
        });
  }

  virtual void WriteF(LogSeverity severity,
                      const char* format,
                      ...) const override {
    va_list args;
    va_start(args, format);
    io_context_.dispatch(
        [logger = logger_, severity,
         formatted_message = base::StringPrintV(format, args)] {
          logger->Write(severity, formatted_message.c_str());
        });
    va_end(args);
  }

 private:
  const std::shared_ptr<const Logger> logger_;
  boost::asio::io_context& io_context_;
};
