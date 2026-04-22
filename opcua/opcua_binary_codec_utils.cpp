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

void AppendBoolean(std::vector<char>& bytes, bool value) {
  AppendUInt8(bytes, value ? 1 : 0);
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

bool ReadBoolean(const std::vector<char>& bytes,
                 std::size_t& offset,
                 bool& value) {
  std::uint8_t raw = 0;
  if (!ReadUInt8(bytes, offset, raw)) {
    return false;
  }
  value = raw != 0;
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

void AppendQualifiedName(std::vector<char>& bytes,
                         const scada::QualifiedName& value) {
  AppendUInt16(bytes, value.namespace_index());
  AppendUaString(bytes, value.name());
}

bool ReadQualifiedName(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::QualifiedName& value) {
  std::uint16_t namespace_index = 0;
  std::string name;
  if (!ReadUInt16(bytes, offset, namespace_index) ||
      !ReadUaString(bytes, offset, name)) {
    return false;
  }
  value = scada::QualifiedName{std::move(name), namespace_index};
  return true;
}

void AppendLocalizedText(std::vector<char>& bytes,
                         const scada::LocalizedText& value) {
  const auto utf8 = ToString(value);
  const std::uint8_t mask = utf8.empty() ? 0 : 0x02;
  AppendUInt8(bytes, mask);
  if ((mask & 0x02) != 0) {
    AppendUaString(bytes, utf8);
  }
}

bool ReadLocalizedText(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::LocalizedText& value) {
  std::uint8_t mask = 0;
  if (!ReadUInt8(bytes, offset, mask)) {
    return false;
  }

  if ((mask & 0x01) != 0) {
    std::string ignored_locale;
    if (!ReadUaString(bytes, offset, ignored_locale)) {
      return false;
    }
  }

  std::string text;
  if ((mask & 0x02) != 0 && !ReadUaString(bytes, offset, text)) {
    return false;
  }
  value = scada::ToLocalizedText(text);
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

void AppendExpandedNodeId(std::vector<char>& bytes,
                          const scada::ExpandedNodeId& node_id) {
  std::uint8_t encoding = 0x00;
  if (!node_id.namespace_uri().empty()) {
    encoding |= 0x80;
  }
  if (node_id.server_index() != 0) {
    encoding |= 0x40;
  }

  const auto* numeric = node_id.node_id().is_numeric() ? &node_id.node_id() : nullptr;
  if (node_id.node_id().is_null()) {
    encoding |= 0x00;
  } else if (numeric != nullptr &&
             numeric->namespace_index() <= 0xff &&
             numeric->numeric_id() <= 0xffff) {
    encoding |= 0x01;
  } else {
    encoding |= 0x02;
  }

  AppendUInt8(bytes, encoding);
  if ((encoding & 0x3f) == 0x01) {
    AppendUInt8(bytes, static_cast<std::uint8_t>(node_id.node_id().namespace_index()));
    AppendUInt16(bytes, static_cast<std::uint16_t>(node_id.node_id().numeric_id()));
  } else if ((encoding & 0x3f) == 0x02) {
    AppendUInt16(bytes, node_id.node_id().namespace_index());
    AppendUInt32(bytes, node_id.node_id().numeric_id());
  }

  if ((encoding & 0x80) != 0) {
    AppendUaString(bytes, node_id.namespace_uri());
  }
  if ((encoding & 0x40) != 0) {
    AppendUInt32(bytes, node_id.server_index());
  }
}

bool ReadExpandedNodeId(const std::vector<char>& bytes,
                        std::size_t& offset,
                        scada::ExpandedNodeId& id) {
  std::uint8_t encoding = 0;
  if (!ReadUInt8(bytes, offset, encoding)) {
    return false;
  }

  scada::NodeId node_id;
  switch (encoding & 0x3f) {
    case 0x00:
      node_id = {};
      break;
    case 0x01: {
      std::uint8_t ns = 0;
      std::uint16_t short_id = 0;
      if (!ReadUInt8(bytes, offset, ns) || !ReadUInt16(bytes, offset, short_id)) {
        return false;
      }
      node_id = scada::NodeId{short_id, ns};
      break;
    }
    case 0x02: {
      std::uint16_t ns = 0;
      std::uint32_t numeric_id = 0;
      if (!ReadUInt16(bytes, offset, ns) ||
          !ReadUInt32(bytes, offset, numeric_id)) {
        return false;
      }
      node_id = scada::NodeId{numeric_id, ns};
      break;
    }
    default:
      return false;
  }

  std::string namespace_uri;
  if ((encoding & 0x80) != 0 && !ReadUaString(bytes, offset, namespace_uri)) {
    return false;
  }

  std::uint32_t server_index = 0;
  if ((encoding & 0x40) != 0 &&
      !ReadUInt32(bytes, offset, server_index)) {
    return false;
  }

  id = scada::ExpandedNodeId{std::move(node_id), std::move(namespace_uri),
                             server_index};
  return true;
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
