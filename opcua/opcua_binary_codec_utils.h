#pragma once

#include "scada/basic_types.h"
#include "scada/node_id.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace opcua::binary {

void AppendUInt8(std::vector<char>& bytes, std::uint8_t value);
void AppendUInt16(std::vector<char>& bytes, std::uint16_t value);
void AppendUInt32(std::vector<char>& bytes, std::uint32_t value);
void AppendInt32(std::vector<char>& bytes, std::int32_t value);
void AppendInt64(std::vector<char>& bytes, std::int64_t value);
void AppendDouble(std::vector<char>& bytes, double value);

bool ReadUInt8(const std::vector<char>& bytes,
               std::size_t& offset,
               std::uint8_t& value);
bool ReadUInt16(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint16_t& value);
bool ReadUInt32(const std::vector<char>& bytes,
                std::size_t& offset,
                std::uint32_t& value);
bool ReadInt32(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int32_t& value);
bool ReadInt64(const std::vector<char>& bytes,
               std::size_t& offset,
               std::int64_t& value);
bool ReadDouble(const std::vector<char>& bytes,
                std::size_t& offset,
                double& value);

void AppendUaString(std::vector<char>& bytes, std::string_view value);
bool ReadUaString(const std::vector<char>& bytes,
                  std::size_t& offset,
                  std::string& value);

void AppendByteString(std::vector<char>& bytes, const scada::ByteString& value);
bool ReadByteString(const std::vector<char>& bytes,
                    std::size_t& offset,
                    scada::ByteString& value);

void AppendNumericNodeId(std::vector<char>& bytes, const scada::NodeId& node_id);
bool ReadNumericNodeId(const std::vector<char>& bytes,
                       std::size_t& offset,
                       scada::NodeId& id);

void AppendExtensionObject(std::vector<char>& bytes,
                           std::uint32_t type_id,
                           const std::vector<char>& body);
bool ReadExtensionObject(const std::vector<char>& bytes,
                         std::size_t& offset,
                         std::uint32_t& type_id,
                         std::uint8_t& encoding,
                         std::vector<char>& body);

void AppendMessage(std::vector<char>& bytes,
                   std::uint32_t type_id,
                   const std::vector<char>& body);
std::optional<std::pair<std::uint32_t, std::vector<char>>> ReadMessage(
    const std::vector<char>& bytes);

}  // namespace opcua::binary
