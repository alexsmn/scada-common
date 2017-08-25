#pragma once

#include "base/bytemsg.h"

#include <stdexcept>
#include <string>

inline void WriteMessageString(ByteMessage& message, const std::string& str) {
  if (str.length() > 255)
    throw std::runtime_error("Too long string");
  message.WriteByte(static_cast<uint8_t>(str.length()));
  message.Write(str.data(), str.length());
}

inline std::string ReadMessageString(ByteMessage& message) {
  size_t length = message.ReadByte();
  const char* data = static_cast<const char*>(message.ptr());
  message.Read(NULL, length);
  return std::string(data, length);
}
