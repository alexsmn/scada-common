#include "opcua/opcua_binary_protocol.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

TEST(OpcUaBinaryProtocolTest, EncodesAndDecodesHelloMessage) {
  const OpcUaBinaryHelloMessage source{
      .protocol_version = 0,
      .receive_buffer_size = 65536,
      .send_buffer_size = 32768,
      .max_message_size = 1048576,
      .max_chunk_count = 16,
      .endpoint_url = "opc.tcp://localhost:4840",
  };

  const auto encoded = EncodeBinaryHelloMessage(source);
  const auto header = DecodeBinaryFrameHeader(encoded);
  ASSERT_TRUE(header.has_value());
  EXPECT_EQ(header->message_type, OpcUaBinaryMessageType::Hello);
  EXPECT_EQ(header->chunk_type, 'F');
  EXPECT_EQ(header->message_size, encoded.size());

  const auto decoded = DecodeBinaryHelloMessage(encoded);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->protocol_version, source.protocol_version);
  EXPECT_EQ(decoded->receive_buffer_size, source.receive_buffer_size);
  EXPECT_EQ(decoded->send_buffer_size, source.send_buffer_size);
  EXPECT_EQ(decoded->max_message_size, source.max_message_size);
  EXPECT_EQ(decoded->max_chunk_count, source.max_chunk_count);
  EXPECT_EQ(decoded->endpoint_url, source.endpoint_url);
}

TEST(OpcUaBinaryProtocolTest, EncodesAndDecodesAcknowledgeMessage) {
  const OpcUaBinaryAcknowledgeMessage source{
      .protocol_version = 0,
      .receive_buffer_size = 32768,
      .send_buffer_size = 16384,
      .max_message_size = 0,
      .max_chunk_count = 0,
  };

  const auto encoded = EncodeBinaryAcknowledgeMessage(source);
  const auto decoded = DecodeBinaryAcknowledgeMessage(encoded);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->protocol_version, source.protocol_version);
  EXPECT_EQ(decoded->receive_buffer_size, source.receive_buffer_size);
  EXPECT_EQ(decoded->send_buffer_size, source.send_buffer_size);
  EXPECT_EQ(decoded->max_message_size, source.max_message_size);
  EXPECT_EQ(decoded->max_chunk_count, source.max_chunk_count);
}

TEST(OpcUaBinaryProtocolTest, EncodesAndDecodesErrorMessage) {
  const OpcUaBinaryErrorMessage source{
      .error = scada::StatusCode::Bad_UnsupportedProtocolVersion,
      .reason = "Unsupported protocol version",
  };

  const auto encoded = EncodeBinaryErrorMessage(source);
  const auto decoded = DecodeBinaryErrorMessage(encoded);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(decoded->error.code(), source.error.code());
  EXPECT_EQ(decoded->reason, source.reason);
}

TEST(OpcUaBinaryProtocolTest, RejectsUnknownFrameHeaderType) {
  std::vector<char> bytes = {'X', 'Y', 'Z', 'F', 8, 0, 0, 0};
  EXPECT_FALSE(DecodeBinaryFrameHeader(bytes).has_value());
}

TEST(OpcUaBinaryProtocolTest, NegotiatesBuffersAgainstPeerLimits) {
  const OpcUaBinaryHelloMessage hello{
      .protocol_version = 3,
      .receive_buffer_size = 8192,
      .send_buffer_size = 4096,
      .max_message_size = 65536,
      .max_chunk_count = 32,
      .endpoint_url = "opc.tcp://localhost:4840",
  };
  const OpcUaBinaryTransportLimits server{
      .protocol_version = 0,
      .receive_buffer_size = 16384,
      .send_buffer_size = 2048,
      .max_message_size = 131072,
      .max_chunk_count = 64,
  };

  const auto negotiated = NegotiateBinaryHello(hello, server);
  ASSERT_TRUE(negotiated.acknowledge.has_value());
  EXPECT_FALSE(negotiated.error.has_value());
  EXPECT_EQ(negotiated.acknowledge->protocol_version, 0u);
  EXPECT_EQ(negotiated.acknowledge->receive_buffer_size, 4096u);
  EXPECT_EQ(negotiated.acknowledge->send_buffer_size, 2048u);
  EXPECT_EQ(negotiated.acknowledge->max_message_size, 131072u);
  EXPECT_EQ(negotiated.acknowledge->max_chunk_count, 64u);
}

TEST(OpcUaBinaryProtocolTest, RejectsZeroPeerBuffers) {
  const OpcUaBinaryHelloMessage hello{
      .protocol_version = 0,
      .receive_buffer_size = 0,
      .send_buffer_size = 8192,
      .max_message_size = 0,
      .max_chunk_count = 0,
      .endpoint_url = "opc.tcp://localhost:4840",
  };

  const auto negotiated =
      NegotiateBinaryHello(hello, OpcUaBinaryTransportLimits{});
  ASSERT_TRUE(negotiated.error.has_value());
  EXPECT_FALSE(negotiated.acknowledge.has_value());
}

}  // namespace
}  // namespace opcua
