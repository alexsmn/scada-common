#pragma once

#include <cassert>
#include <cstdint>
#include <string>

namespace protocol {

typedef uint32_t MessageSizeType;

inline size_t GetMessagePayloadSize(const std::string& message) {
  assert(message.size() >= sizeof(MessageSizeType));
  return reinterpret_cast<const MessageSizeType&>(message[0]);
}

inline const void* GetMessagePayload(const std::string& message) {
  assert(message.size() >= sizeof(MessageSizeType));
  return &message[sizeof(MessageSizeType)];
}

inline size_t GetIncomingMessageSize(const std::string& message) {
  if (message.size() < sizeof(MessageSizeType))
    return sizeof(MessageSizeType);
  return sizeof(MessageSizeType) + GetMessagePayloadSize(message);
}

inline void AppendMessage(std::string& message, const void* data, size_t size) {
  const char* bytes = reinterpret_cast<const char*>(data);
  message.insert(message.end(), bytes, bytes + size);
}

inline void PrependMessageSize(std::string& message) {
  assert(message.empty());
  message.resize(sizeof(MessageSizeType));
}

inline void UpdateMessageSize(std::string& message) {
  reinterpret_cast<MessageSizeType&>(message[0]) =
      message.size() - sizeof(MessageSizeType);
}

} // namespace protocol