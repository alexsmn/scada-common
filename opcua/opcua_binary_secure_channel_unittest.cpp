#include "opcua/opcua_binary_secure_channel.h"

#include "base/test/awaitable_test.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

const std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();

std::vector<char> EncodeOpenRequestBody(
    std::uint32_t request_handle,
    OpcUaBinarySecurityTokenRequestType request_type =
        OpcUaBinarySecurityTokenRequestType::Issue,
    OpcUaBinaryMessageSecurityMode security_mode =
        OpcUaBinaryMessageSecurityMode::None,
    std::uint32_t requested_lifetime = 60000) {
  auto append_u8 = [](std::vector<char>& bytes, std::uint8_t value) {
    bytes.push_back(static_cast<char>(value));
  };
  auto append_u16 = [](std::vector<char>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<char>(value & 0xff));
    bytes.push_back(static_cast<char>((value >> 8) & 0xff));
  };
  auto append_u32 = [](std::vector<char>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<char>(value & 0xff));
    bytes.push_back(static_cast<char>((value >> 8) & 0xff));
    bytes.push_back(static_cast<char>((value >> 16) & 0xff));
    bytes.push_back(static_cast<char>((value >> 24) & 0xff));
  };
  auto append_i32 = [&](std::vector<char>& bytes, std::int32_t value) {
    append_u32(bytes, static_cast<std::uint32_t>(value));
  };
  auto append_i64 = [](std::vector<char>& bytes, std::int64_t value) {
    const auto raw = static_cast<std::uint64_t>(value);
    for (int i = 0; i < 8; ++i) {
      bytes.push_back(static_cast<char>((raw >> (8 * i)) & 0xff));
    }
  };
  auto append_string = [&](std::vector<char>& bytes, std::string_view value) {
    append_i32(bytes, static_cast<std::int32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
  };
  auto append_bytes = [&](std::vector<char>& bytes,
                          const scada::ByteString& value) {
    append_i32(bytes, static_cast<std::int32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
  };
  auto append_nodeid = [&](std::vector<char>& bytes, std::uint32_t id) {
    if (id == 0) {
      append_u8(bytes, 0x00);
    } else {
      append_u8(bytes, 0x01);
      append_u8(bytes, 0);
      append_u16(bytes, static_cast<std::uint16_t>(id));
    }
  };
  auto append_extension = [&](std::vector<char>& bytes,
                              std::uint32_t type_id,
                              const std::vector<char>& payload) {
    append_nodeid(bytes, type_id);
    append_u8(bytes, 0x01);
    append_i32(bytes, static_cast<std::int32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
  };

  std::vector<char> payload;
  append_nodeid(payload, 0);
  append_i64(payload, 0);
  append_u32(payload, request_handle);
  append_u32(payload, 0);
  append_string(payload, "");
  append_u32(payload, 0);
  append_nodeid(payload, 0);
  append_u8(payload, 0x00);
  append_u32(payload, 0);
  append_u32(payload, static_cast<std::uint32_t>(request_type));
  append_u32(payload, static_cast<std::uint32_t>(security_mode));
  append_bytes(payload, {});
  append_u32(payload, requested_lifetime);

  std::vector<char> body;
  append_extension(body, kOpenSecureChannelRequestBinaryEncodingId, payload);
  return body;
}

std::vector<char> EncodeCloseRequestBody(std::uint32_t request_handle) {
  auto append_u8 = [](std::vector<char>& bytes, std::uint8_t value) {
    bytes.push_back(static_cast<char>(value));
  };
  auto append_u16 = [](std::vector<char>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<char>(value & 0xff));
    bytes.push_back(static_cast<char>((value >> 8) & 0xff));
  };
  auto append_u32 = [](std::vector<char>& bytes, std::uint32_t value) {
    bytes.push_back(static_cast<char>(value & 0xff));
    bytes.push_back(static_cast<char>((value >> 8) & 0xff));
    bytes.push_back(static_cast<char>((value >> 16) & 0xff));
    bytes.push_back(static_cast<char>((value >> 24) & 0xff));
  };
  auto append_i32 = [&](std::vector<char>& bytes, std::int32_t value) {
    append_u32(bytes, static_cast<std::uint32_t>(value));
  };
  auto append_i64 = [](std::vector<char>& bytes, std::int64_t value) {
    const auto raw = static_cast<std::uint64_t>(value);
    for (int i = 0; i < 8; ++i) {
      bytes.push_back(static_cast<char>((raw >> (8 * i)) & 0xff));
    }
  };
  auto append_string = [&](std::vector<char>& bytes, std::string_view value) {
    append_i32(bytes, static_cast<std::int32_t>(value.size()));
    bytes.insert(bytes.end(), value.begin(), value.end());
  };
  auto append_nodeid = [&](std::vector<char>& bytes, std::uint32_t id) {
    if (id == 0) {
      append_u8(bytes, 0x00);
    } else {
      append_u8(bytes, 0x01);
      append_u8(bytes, 0);
      append_u16(bytes, static_cast<std::uint16_t>(id));
    }
  };
  auto append_extension = [&](std::vector<char>& bytes,
                              std::uint32_t type_id,
                              const std::vector<char>& payload) {
    append_nodeid(bytes, type_id);
    append_u8(bytes, 0x01);
    append_i32(bytes, static_cast<std::int32_t>(payload.size()));
    bytes.insert(bytes.end(), payload.begin(), payload.end());
  };

  std::vector<char> payload;
  append_nodeid(payload, 0);
  append_i64(payload, 0);
  append_u32(payload, request_handle);
  append_u32(payload, 0);
  append_string(payload, "");
  append_u32(payload, 0);
  append_nodeid(payload, 0);
  append_u8(payload, 0x00);

  std::vector<char> body;
  append_extension(body, kCloseSecureChannelRequestBinaryEncodingId, payload);
  return body;
}

TEST(OpcUaBinarySecureChannelTest, DecodesAndEncodesOpenRequestResponse) {
  const auto body = EncodeOpenRequestBody(77);
  const auto request = DecodeOpenSecureChannelRequestBody(body);
  ASSERT_TRUE(request.has_value());
  EXPECT_EQ(request->request_header.request_handle, 77u);
  EXPECT_EQ(request->security_mode, OpcUaBinaryMessageSecurityMode::None);

  const auto response_body = EncodeOpenSecureChannelResponseBody(
      {.response_header = {.request_handle = 77,
                           .service_result = scada::StatusCode::Good},
       .server_protocol_version = 0,
       .security_token =
           {.channel_id = 91, .token_id = 4, .created_at = 0, .revised_lifetime = 60000},
       .server_nonce = {}});
  EXPECT_FALSE(response_body.empty());
}

TEST(OpcUaBinarySecureChannelTest, OpenRequestProducesOpenResponseFrame) {
  OpcUaBinarySecureChannel channel{91};
  const auto frame = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureOpen,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 0,
       .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
           .security_policy_uri = std::string{kSecurityPolicyNone},
           .sender_certificate = {},
           .receiver_certificate_thumbprint = {},
       },
       .sequence_header = {.sequence_number = 1, .request_id = 8},
       .body = EncodeOpenRequestBody(55)});

  const auto result = WaitAwaitable(executor_, channel.HandleFrame(frame));
  ASSERT_TRUE(result.outbound_frame.has_value());
  EXPECT_FALSE(result.close_transport);
  EXPECT_TRUE(channel.opened());

  const auto response = DecodeSecureConversationMessage(*result.outbound_frame);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->frame_header.message_type,
            OpcUaBinaryMessageType::SecureOpen);
  EXPECT_EQ(response->secure_channel_id, 91u);
  EXPECT_EQ(response->sequence_header.request_id, 8u);
}

TEST(OpcUaBinarySecureChannelTest, RejectsUnsupportedSecurityModeInOpen) {
  OpcUaBinarySecureChannel channel{19};
  const auto frame = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureOpen,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 0,
       .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
           .security_policy_uri = std::string{kSecurityPolicyNone},
           .sender_certificate = {},
           .receiver_certificate_thumbprint = {},
       },
       .sequence_header = {.sequence_number = 1, .request_id = 2},
       .body = EncodeOpenRequestBody(
           12, OpcUaBinarySecurityTokenRequestType::Issue,
           OpcUaBinaryMessageSecurityMode::Sign)});

  const auto result = WaitAwaitable(executor_, channel.HandleFrame(frame));
  ASSERT_TRUE(result.outbound_frame.has_value());
  EXPECT_FALSE(channel.opened());
}

TEST(OpcUaBinarySecureChannelTest, RoutesMessageBodyAfterOpen) {
  OpcUaBinarySecureChannel channel{7};
  const auto open_frame = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureOpen,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 0,
       .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
           .security_policy_uri = std::string{kSecurityPolicyNone},
           .sender_certificate = {},
           .receiver_certificate_thumbprint = {},
       },
       .sequence_header = {.sequence_number = 1, .request_id = 4},
       .body = EncodeOpenRequestBody(44)});
  ASSERT_TRUE(WaitAwaitable(executor_, channel.HandleFrame(open_frame))
                  .outbound_frame.has_value());

  const std::vector<char> payload{'x', 'y', 'z'};
  const auto msg_frame = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureMessage,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 7,
       .symmetric_security_header = OpcUaBinarySymmetricSecurityHeader{
           .token_id = channel.token_id()},
       .sequence_header = {.sequence_number = 2, .request_id = 5},
       .body = payload});

  const auto result = WaitAwaitable(executor_, channel.HandleFrame(msg_frame));
  ASSERT_TRUE(result.service_payload.has_value());
  EXPECT_EQ(*result.service_payload, payload);
  ASSERT_TRUE(result.request_id.has_value());
  EXPECT_EQ(*result.request_id, 5u);

  const auto response_frame = channel.BuildServiceResponse(
      *result.request_id, std::vector<char>{'o', 'k'});
  const auto response = DecodeSecureConversationMessage(response_frame);
  ASSERT_TRUE(response.has_value());
  EXPECT_EQ(response->frame_header.message_type,
            OpcUaBinaryMessageType::SecureMessage);
  EXPECT_EQ(response->sequence_header.request_id, 5u);
  EXPECT_EQ(response->body, (std::vector<char>{'o', 'k'}));
}

TEST(OpcUaBinarySecureChannelTest, CloseRequestClosesTransport) {
  OpcUaBinarySecureChannel channel{11};
  const auto open_frame = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureOpen,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 0,
       .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
           .security_policy_uri = std::string{kSecurityPolicyNone},
           .sender_certificate = {},
           .receiver_certificate_thumbprint = {},
       },
       .sequence_header = {.sequence_number = 1, .request_id = 1},
       .body = EncodeOpenRequestBody(1)});
  ASSERT_TRUE(WaitAwaitable(executor_, channel.HandleFrame(open_frame))
                  .outbound_frame.has_value());

  const auto close_frame = EncodeSecureConversationMessage(
      {.frame_header =
           {.message_type = OpcUaBinaryMessageType::SecureClose,
            .chunk_type = 'F',
            .message_size = 0},
       .secure_channel_id = 11,
       .symmetric_security_header = OpcUaBinarySymmetricSecurityHeader{
           .token_id = channel.token_id()},
       .sequence_header = {.sequence_number = 2, .request_id = 2},
       .body = EncodeCloseRequestBody(2)});

  const auto result = WaitAwaitable(executor_, channel.HandleFrame(close_frame));
  EXPECT_TRUE(result.close_transport);
}
}  // namespace
}  // namespace opcua
