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

struct EncodedExtensionObject {
  std::uint32_t type_id = 0;
  std::vector<char> body;
};

struct DecodedExtensionObject {
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> body;
};

class BinaryEncoder {
 public:
  explicit BinaryEncoder(std::vector<char>& bytes) : bytes_{bytes} {}

  void Encode(std::uint8_t value);
  void Encode(std::uint16_t value);
  void Encode(std::uint32_t value);
  void Encode(bool value);
  void Encode(std::int32_t value);
  void Encode(std::int64_t value);
  void Encode(double value);

  void Encode(std::string_view value);
  void Encode(const scada::String& value);
  void Encode(const scada::QualifiedName& value);
  void Encode(const scada::LocalizedText& value);
  void Encode(const scada::ByteString& value);
  void Encode(const scada::NodeId& node_id);
  void Encode(const scada::ExpandedNodeId& node_id);
  void Encode(const scada::Variant& value);
  void Encode(const EncodedExtensionObject& value);

  std::vector<char>& bytes() { return bytes_; }

 private:
  std::vector<char>& bytes_;
};

class BinaryDecoder {
 public:
  explicit BinaryDecoder(std::span<const char> bytes) : bytes_{bytes} {}
  explicit BinaryDecoder(const std::vector<char>& bytes) : bytes_{bytes} {}

  bool Decode(std::uint8_t& value);
  bool Decode(std::uint16_t& value);
  bool Decode(std::uint32_t& value);
  bool Decode(bool& value);
  bool Decode(std::int32_t& value);
  bool Decode(std::int64_t& value);
  bool Decode(double& value);

  bool Decode(scada::String& value);
  bool Decode(scada::QualifiedName& value);
  bool Decode(scada::LocalizedText& value);
  bool Decode(scada::ByteString& value);
  bool Decode(scada::NodeId& id);
  bool Decode(scada::ExpandedNodeId& id);
  bool Decode(scada::Variant& value);
  bool Decode(DecodedExtensionObject& value);

  std::size_t offset() const { return offset_; }
  bool consumed() const { return offset_ == bytes_.size(); }
  std::span<const char> remaining() const { return bytes_.subspan(offset_); }

 private:
  std::span<const char> bytes_;
  std::size_t offset_ = 0;
};

void AppendMessage(std::vector<char>& bytes,
                   std::uint32_t type_id,
                   std::span<const char> payload);
std::optional<std::pair<std::uint32_t, std::vector<char>>> ReadMessage(
    const std::vector<char>& bytes);

}  // namespace opcua::binary
