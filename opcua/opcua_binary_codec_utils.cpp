#include "opcua/opcua_binary_codec_utils.h"

#include <cstring>

namespace opcua::binary {

void AppendUInt8(std::vector<char>& bytes, std::uint8_t value) {
  bytes.push_back(static_cast<char>(value));
}

void AppendUInt16(std::vector<char>& bytes, std::uint16_t value) {
  bytes.push_back(static_cast<char>(value & 0xff));
  bytes.push_back(static_cast<char>((value >> 8) & 0xff));
}

void AppendUInt32(std::vector<char>& bytes, std::uint32_t value) {
  bytes.push_back(static_cast<char>(value & 0xff));
  bytes.push_back(static_cast<char>((value >> 8) & 0xff));
  bytes.push_back(static_cast<char>((value >> 16) & 0xff));
  bytes.push_back(static_cast<char>((value >> 24) & 0xff));
}

void AppendInt32(std::vector<char>& bytes, std::int32_t value) {
  AppendUInt32(bytes, static_cast<std::uint32_t>(value));
}

void AppendInt64(std::vector<char>& bytes, std::int64_t value) {
  const auto raw = static_cast<std::uint64_t>(value);
  for (int i = 0; i < 8; ++i) {
    bytes.push_back(static_cast<char>((raw >> (8 * i)) & 0xff));
  }
}

void AppendDouble(std::vector<char>& bytes, double value) {
  const auto* raw = reinterpret_cast<const char*>(&value);
  bytes.insert(bytes.end(), raw, raw + sizeof(value));
}

bool ReadUInt8(const std::vector<char>& bytes,
               std::size_t& offset,
               std::uint8_t& value) {
  if (offset + 1 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint8_t>(bytes[offset]);
  ++offset;
  return true;
}

bool ReadUInt16(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint16_t& value) {
  if (offset + 2 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint16_t>(
      static_cast<unsigned char>(bytes[offset]) |
      (static_cast<std::uint16_t>(static_cast<unsigned char>(bytes[offset + 1]))
       << 8));
  offset += 2;
  return true;
}

bool ReadUInt32(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint32_t& value) {
  if (offset + 4 > bytes.size()) {
    return false;
  }
  value = static_cast<std::uint32_t>(
              static_cast<unsigned char>(bytes[offset])) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 1]))
           << 8) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 2]))
           << 16) |
          (static_cast<std::uint32_t>(
               static_cast<unsigned char>(bytes[offset + 3]))
           << 24);
  offset += 4;
  return true;
}

bool ReadInt32(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int32_t& value) {
  std::uint32_t raw = 0;
  if (!ReadUInt32(bytes, offset, raw)) {
    return false;
  }
  value = static_cast<std::int32_t>(raw);
  return true;
}

bool ReadInt64(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int64_t& value) {
  if (offset + 8 > bytes.size()) {
    return false;
  }
  std::uint64_t raw = 0;
  for (int i = 0; i < 8; ++i) {
    raw |= static_cast<std::uint64_t>(
               static_cast<unsigned char>(bytes[offset + i]))
           << (8 * i);
  }
  value = static_cast<std::int64_t>(raw);
  offset += 8;
  return true;
}

bool ReadDouble(const std::vector<char>& bytes,
                std::size_t& offset,
                double& value) {
  if (offset + sizeof(double) > bytes.size()) {
    return false;
  }
  std::memcpy(&value, bytes.data() + offset, sizeof(value));
  offset += sizeof(value);
  return true;
}

void AppendUaString(std::vector<char>& bytes, std::string_view value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

bool ReadUaString(const std::vector<char>& bytes,
                  std::size_t& offset,
                  std::string& value) {
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length)) {
    return false;
  }
  if (length < 0) {
    value.clear();
    return true;
  }
  if (offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  value.assign(bytes.data() + offset, static_cast<std::size_t>(length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendByteString(std::vector<char>& bytes, const scada::ByteString& value) {
  AppendInt32(bytes, static_cast<std::int32_t>(value.size()));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

bool ReadByteString(const std::vector<char>& bytes,
                    std::size_t& offset,
                    scada::ByteString& value) {
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length)) {
    return false;
  }
  if (length < 0) {
    value.clear();
    return true;
  }
  if (offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  value.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
               bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendNumericNodeId(std::vector<char>& bytes, const scada::NodeId& node_id) {
  if (node_id.is_null()) {
    AppendUInt8(bytes, 0x00);
    return;
  }
  if (!node_id.is_numeric()) {
    AppendUInt8(bytes, 0x00);
    return;
  }
  if (node_id.namespace_index() <= 0xff && node_id.numeric_id() <= 0xffff) {
    AppendUInt8(bytes, 0x01);
    AppendUInt8(bytes, static_cast<std::uint8_t>(node_id.namespace_index()));
    AppendUInt16(bytes, static_cast<std::uint16_t>(node_id.numeric_id()));
    return;
  }
  AppendUInt8(bytes, 0x02);
  AppendUInt16(bytes, node_id.namespace_index());
  AppendUInt32(bytes, node_id.numeric_id());
}

bool ReadNumericNodeId(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::NodeId& id) {
  std::uint8_t encoding = 0;
  if (!ReadUInt8(bytes, offset, encoding)) {
    return false;
  }
  if (encoding == 0x00) {
    id = {};
    return true;
  }
  if (encoding == 0x01) {
    std::uint8_t ns = 0;
    std::uint16_t short_id = 0;
    if (!ReadUInt8(bytes, offset, ns) || !ReadUInt16(bytes, offset, short_id)) {
      return false;
    }
    id = scada::NodeId{short_id, ns};
    return true;
  }
  if (encoding == 0x02) {
    std::uint16_t ns = 0;
    std::uint32_t numeric_id = 0;
    if (!ReadUInt16(bytes, offset, ns) || !ReadUInt32(bytes, offset, numeric_id)) {
      return false;
    }
    id = scada::NodeId{numeric_id, ns};
    return true;
  }
  return false;
}

void AppendExtensionObject(std::vector<char>& bytes,
                           std::uint32_t type_id,
                           const std::vector<char>& body) {
  AppendNumericNodeId(bytes, scada::NodeId{type_id});
  AppendUInt8(bytes, 0x01);
  AppendInt32(bytes, static_cast<std::int32_t>(body.size()));
  bytes.insert(bytes.end(), body.begin(), body.end());
}

bool ReadExtensionObject(const std::vector<char>& bytes,
                         std::size_t& offset,
                         std::uint32_t& type_id,
                         std::uint8_t& encoding,
                         std::vector<char>& body) {
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(bytes, offset, type_node_id) ||
      !type_node_id.is_numeric() ||
      !ReadUInt8(bytes, offset, encoding)) {
    return false;
  }
  type_id = type_node_id.numeric_id();
  if (encoding == 0x00) {
    body.clear();
    return true;
  }
  std::int32_t length = 0;
  if (!ReadInt32(bytes, offset, length) || length < 0 ||
      offset + static_cast<std::size_t>(length) > bytes.size()) {
    return false;
  }
  body.assign(bytes.begin() + static_cast<std::ptrdiff_t>(offset),
              bytes.begin() + static_cast<std::ptrdiff_t>(offset + length));
  offset += static_cast<std::size_t>(length);
  return true;
}

void AppendMessage(std::vector<char>& bytes,
                   std::uint32_t type_id,
                   const std::vector<char>& body) {
  AppendNumericNodeId(bytes, scada::NodeId{type_id});
  bytes.insert(bytes.end(), body.begin(), body.end());
}

std::optional<std::pair<std::uint32_t, std::vector<char>>> ReadMessage(
    const std::vector<char>& bytes) {
  std::size_t offset = 0;
  scada::NodeId type_node_id;
  if (!ReadNumericNodeId(bytes, offset, type_node_id) ||
      !type_node_id.is_numeric()) {
    return std::nullopt;
  }

  return std::pair{type_node_id.numeric_id(),
                   std::vector<char>{bytes.begin() +
                                         static_cast<std::ptrdiff_t>(offset),
                                     bytes.end()}};
}

}  // namespace opcua::binary
