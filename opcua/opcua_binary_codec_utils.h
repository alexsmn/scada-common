#pragma once

#include "scada/basic_types.h"
#include "scada/expanded_node_id.h"
#include "scada/localized_text.h"
#include "scada/node_id.h"
#include "scada/qualified_name.h"
#include "scada/variant.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opcua::binary {

class BinaryEncoder {
 public:
  explicit BinaryEncoder(std::vector<char>& bytes) : bytes_{bytes} {}

  void AppendUInt8(std::uint8_t value);
  void AppendUInt16(std::uint16_t value);
  void AppendUInt32(std::uint32_t value);
  void AppendBoolean(bool value);
  void AppendInt32(std::int32_t value);
  void AppendInt64(std::int64_t value);
  void AppendDouble(double value);

  void AppendUaString(std::string_view value);
  void AppendQualifiedName(const scada::QualifiedName& value);
  void AppendLocalizedText(const scada::LocalizedText& value);
  void AppendByteString(const scada::ByteString& value);
  void AppendNumericNodeId(const scada::NodeId& node_id);
  void AppendExpandedNodeId(const scada::ExpandedNodeId& node_id);
  void AppendVariant(const scada::Variant& value);
  void AppendExtensionObject(std::uint32_t type_id,
                             const std::vector<char>& body);
  void AppendMessage(std::uint32_t type_id, const std::vector<char>& body);

  std::vector<char>& bytes() { return bytes_; }

 private:
  std::vector<char>& bytes_;
};

class BinaryDecoder {
 public:
  explicit BinaryDecoder(std::span<const char> bytes) : bytes_{bytes} {}
  explicit BinaryDecoder(const std::vector<char>& bytes) : bytes_{bytes} {}

  bool ReadUInt8(std::uint8_t& value);
  bool ReadUInt16(std::uint16_t& value);
  bool ReadUInt32(std::uint32_t& value);
  bool ReadBoolean(bool& value);
  bool ReadInt32(std::int32_t& value);
  bool ReadInt64(std::int64_t& value);
  bool ReadDouble(double& value);

  bool ReadUaString(std::string& value);
  bool ReadQualifiedName(scada::QualifiedName& value);
  bool ReadLocalizedText(scada::LocalizedText& value);
  bool ReadByteString(scada::ByteString& value);
  bool ReadNumericNodeId(scada::NodeId& id);
  bool ReadExpandedNodeId(scada::ExpandedNodeId& id);
  bool ReadVariant(scada::Variant& value);
  bool ReadExtensionObject(std::uint32_t& type_id,
                           std::uint8_t& encoding,
                           std::vector<char>& body);

  std::size_t offset() const { return offset_; }
  bool consumed() const { return offset_ == bytes_.size(); }
  std::span<const char> remaining() const { return bytes_.subspan(offset_); }

 private:
  std::span<const char> bytes_;
  std::size_t offset_ = 0;
};

std::optional<std::pair<std::uint32_t, std::vector<char>>> ReadMessage(
    const std::vector<char>& bytes);

}  // namespace opcua::binary
