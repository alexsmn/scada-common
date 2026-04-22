#include "opcua/opcua_binary_codec_utils.h"

#include <gtest/gtest.h>

namespace opcua::binary {
namespace {

TEST(OpcUaBinaryCodecUtilsTest, RoundTripsPrimitiveValues) {
  std::vector<char> bytes;
  AppendUInt8(bytes, 0xab);
  AppendUInt16(bytes, 0x1234);
  AppendUInt32(bytes, 0x89abcdef);
  AppendBoolean(bytes, true);
  AppendInt32(bytes, -1234567);
  AppendInt64(bytes, -9876543210LL);
  AppendDouble(bytes, 12.5);

  std::size_t offset = 0;
  std::uint8_t u8 = 0;
  std::uint16_t u16 = 0;
  std::uint32_t u32 = 0;
  bool boolean = false;
  std::int32_t i32 = 0;
  std::int64_t i64 = 0;
  double dbl = 0;
  EXPECT_TRUE(ReadUInt8(bytes, offset, u8));
  EXPECT_TRUE(ReadUInt16(bytes, offset, u16));
  EXPECT_TRUE(ReadUInt32(bytes, offset, u32));
  EXPECT_TRUE(ReadBoolean(bytes, offset, boolean));
  EXPECT_TRUE(ReadInt32(bytes, offset, i32));
  EXPECT_TRUE(ReadInt64(bytes, offset, i64));
  EXPECT_TRUE(ReadDouble(bytes, offset, dbl));

  EXPECT_EQ(u8, 0xab);
  EXPECT_EQ(u16, 0x1234);
  EXPECT_EQ(u32, 0x89abcdefu);
  EXPECT_TRUE(boolean);
  EXPECT_EQ(i32, -1234567);
  EXPECT_EQ(i64, -9876543210LL);
  EXPECT_DOUBLE_EQ(dbl, 12.5);
  EXPECT_EQ(offset, bytes.size());
}

TEST(OpcUaBinaryCodecUtilsTest, HandlesStringsAndByteStrings) {
  std::vector<char> bytes;
  AppendUaString(bytes, "opc.tcp://localhost:4840");
  AppendQualifiedName(bytes, scada::QualifiedName{"BrowseName", 2});
  AppendLocalizedText(bytes, scada::ToLocalizedText(u"DisplayName"));
  AppendByteString(bytes, scada::ByteString{'a', 'b', 'c'});
  AppendInt32(bytes, -1);
  AppendInt32(bytes, -1);

  std::size_t offset = 0;
  std::string string_value;
  scada::QualifiedName qualified_name;
  scada::LocalizedText localized_text;
  scada::ByteString byte_string;
  std::string null_string = "sentinel";
  scada::ByteString null_bytes = {'x'};
  EXPECT_TRUE(ReadUaString(bytes, offset, string_value));
  EXPECT_TRUE(ReadQualifiedName(bytes, offset, qualified_name));
  EXPECT_TRUE(ReadLocalizedText(bytes, offset, localized_text));
  EXPECT_TRUE(ReadByteString(bytes, offset, byte_string));
  EXPECT_TRUE(ReadUaString(bytes, offset, null_string));
  EXPECT_TRUE(ReadByteString(bytes, offset, null_bytes));

  EXPECT_EQ(string_value, "opc.tcp://localhost:4840");
  EXPECT_EQ(qualified_name, (scada::QualifiedName{"BrowseName", 2}));
  EXPECT_EQ(localized_text, scada::ToLocalizedText(u"DisplayName"));
  EXPECT_EQ(byte_string, (scada::ByteString{'a', 'b', 'c'}));
  EXPECT_TRUE(null_string.empty());
  EXPECT_TRUE(null_bytes.empty());
  EXPECT_EQ(offset, bytes.size());
}

TEST(OpcUaBinaryCodecUtilsTest, RoundTripsNumericNodeIds) {
  std::vector<char> bytes;
  AppendNumericNodeId(bytes, scada::NodeId{});
  AppendNumericNodeId(bytes, scada::NodeId{255, 2});
  AppendNumericNodeId(bytes, scada::NodeId{70000, 513});
  AppendExpandedNodeId(
      bytes, scada::ExpandedNodeId{scada::NodeId{42, 2}, "urn:test", 7});

  std::size_t offset = 0;
  scada::NodeId null_id;
  scada::NodeId small_id;
  scada::NodeId large_id;
  scada::ExpandedNodeId expanded_id;
  EXPECT_TRUE(ReadNumericNodeId(bytes, offset, null_id));
  EXPECT_TRUE(ReadNumericNodeId(bytes, offset, small_id));
  EXPECT_TRUE(ReadNumericNodeId(bytes, offset, large_id));
  EXPECT_TRUE(ReadExpandedNodeId(bytes, offset, expanded_id));

  EXPECT_TRUE(null_id.is_null());
  EXPECT_EQ(small_id, (scada::NodeId{255, 2}));
  EXPECT_EQ(large_id, (scada::NodeId{70000, 513}));
  EXPECT_EQ(expanded_id,
            (scada::ExpandedNodeId{scada::NodeId{42, 2}, "urn:test", 7}));
  EXPECT_EQ(offset, bytes.size());
}

TEST(OpcUaBinaryCodecUtilsTest, RoundTripsExtensionObjectsAndMessages) {
  const std::vector<char> body{'x', 'y', 'z'};

  std::vector<char> extension_bytes;
  AppendExtensionObject(extension_bytes, 324, body);

  std::size_t offset = 0;
  std::uint32_t type_id = 0;
  std::uint8_t encoding = 0;
  std::vector<char> decoded_body;
  ASSERT_TRUE(
      ReadExtensionObject(extension_bytes, offset, type_id, encoding, decoded_body));
  EXPECT_EQ(type_id, 324u);
  EXPECT_EQ(encoding, 0x01);
  EXPECT_EQ(decoded_body, body);
  EXPECT_EQ(offset, extension_bytes.size());

  std::vector<char> message_bytes;
  AppendMessage(message_bytes, 629, body);
  const auto message = ReadMessage(message_bytes);
  ASSERT_TRUE(message.has_value());
  EXPECT_EQ(message->first, 629u);
  EXPECT_EQ(message->second, body);
}

TEST(OpcUaBinaryCodecUtilsTest, RejectsTruncatedPayloads) {
  std::vector<char> truncated_string;
  AppendInt32(truncated_string, 4);
  truncated_string.push_back('o');

  std::size_t offset = 0;
  std::string value;
  EXPECT_FALSE(ReadUaString(truncated_string, offset, value));

  std::vector<char> invalid_message{0x03};
  EXPECT_FALSE(ReadMessage(invalid_message).has_value());
}

}  // namespace
}  // namespace opcua::binary
