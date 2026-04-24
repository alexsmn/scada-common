#include "opcua/binary/client/opcua_binary_client_secure_channel.h"

#include <utility>

namespace opcua {

OpcUaBinaryClientSecureChannel::OpcUaBinaryClientSecureChannel(
    OpcUaBinaryClientTransport& transport)
    : transport_{transport} {}

Awaitable<scada::Status> OpcUaBinaryClientSecureChannel::Open(
    std::uint32_t requested_lifetime_ms) {
  // Build the OpenSecureChannel request body, wrap it in an asymmetric
  // SecureConversationMessage frame with SecurityPolicy=None, and write.
  const std::uint32_t request_id = NextRequestId();
  const OpcUaBinaryOpenSecureChannelRequest request{
      .request_header = {.request_handle = request_id},
      .client_protocol_version = 0,
      .request_type = OpcUaBinarySecurityTokenRequestType::Issue,
      .security_mode = OpcUaBinaryMessageSecurityMode::None,
      .client_nonce = {},
      .requested_lifetime = requested_lifetime_ms,
  };
  const auto body = EncodeOpenSecureChannelRequestBody(request);

  const OpcUaBinarySecureConversationMessage frame_message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureOpen,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = 0,
      .asymmetric_security_header = OpcUaBinaryAsymmetricSecurityHeader{
          .security_policy_uri = std::string{kSecurityPolicyNone},
          .sender_certificate = {},
          .receiver_certificate_thumbprint = {},
      },
      .sequence_header = {.sequence_number = next_sequence_number_++,
                          .request_id = request_id},
      .body = body,
  };
  const auto frame = EncodeSecureConversationMessage(frame_message);
  auto write_status = co_await transport_.WriteFrame(frame);
  if (write_status.bad()) {
    co_return write_status;
  }

  auto read_frame = co_await transport_.ReadFrame();
  if (!read_frame.ok()) {
    co_return read_frame.status();
  }

  const auto response_message = DecodeSecureConversationMessage(*read_frame);
  if (!response_message.has_value() ||
      response_message->frame_header.message_type !=
          OpcUaBinaryMessageType::SecureOpen ||
      !response_message->asymmetric_security_header.has_value() ||
      response_message->asymmetric_security_header->security_policy_uri !=
          kSecurityPolicyNone) {
    co_return scada::Status{scada::StatusCode::Bad};
  }

  const auto response =
      DecodeOpenSecureChannelResponseBody(response_message->body);
  if (!response.has_value() || response->response_header.service_result.bad()) {
    co_return response.has_value() ? response->response_header.service_result
                                   : scada::Status{scada::StatusCode::Bad};
  }

  channel_id_ = response->security_token.channel_id;
  token_id_ = response->security_token.token_id;
  opened_ = true;
  co_return scada::Status{scada::StatusCode::Good};
}

Awaitable<scada::Status> OpcUaBinaryClientSecureChannel::SendServiceRequest(
    std::uint32_t request_id,
    const std::vector<char>& body) {
  if (!opened_) {
    co_return scada::Status{scada::StatusCode::Bad_Disconnected};
  }
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureMessage,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = channel_id_,
      .asymmetric_security_header = std::nullopt,
      .symmetric_security_header =
          OpcUaBinarySymmetricSecurityHeader{.token_id = token_id_},
      .sequence_header = {.sequence_number = next_sequence_number_++,
                          .request_id = request_id},
      .body = body,
  };
  co_return co_await transport_.WriteFrame(
      EncodeSecureConversationMessage(message));
}

Awaitable<scada::StatusOr<OpcUaBinaryClientSecureChannel::ServiceResponse>>
OpcUaBinaryClientSecureChannel::ReadServiceResponse() {
  if (!opened_) {
    co_return scada::StatusOr<ServiceResponse>{
        scada::Status{scada::StatusCode::Bad_Disconnected}};
  }
  auto frame = co_await transport_.ReadFrame();
  if (!frame.ok()) {
    co_return scada::StatusOr<ServiceResponse>{frame.status()};
  }
  const auto message = DecodeSecureConversationMessage(*frame);
  if (!message.has_value() ||
      message->frame_header.message_type !=
          OpcUaBinaryMessageType::SecureMessage ||
      message->secure_channel_id != channel_id_ ||
      !message->symmetric_security_header.has_value() ||
      message->symmetric_security_header->token_id != token_id_) {
    co_return scada::StatusOr<ServiceResponse>{
        scada::Status{scada::StatusCode::Bad}};
  }
  co_return scada::StatusOr<ServiceResponse>{
      ServiceResponse{.request_id = message->sequence_header.request_id,
                      .body = std::move(message->body)}};
}

Awaitable<scada::Status> OpcUaBinaryClientSecureChannel::Close() {
  if (!opened_) {
    co_return scada::Status{scada::StatusCode::Good};
  }
  const std::uint32_t request_id = NextRequestId();
  const OpcUaBinaryCloseSecureChannelRequest close_request{
      .request_header = {.request_handle = request_id}};
  const auto body = EncodeCloseSecureChannelRequestBody(close_request);
  const OpcUaBinarySecureConversationMessage message{
      .frame_header = {.message_type = OpcUaBinaryMessageType::SecureClose,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = channel_id_,
      .asymmetric_security_header = std::nullopt,
      .symmetric_security_header =
          OpcUaBinarySymmetricSecurityHeader{.token_id = token_id_},
      .sequence_header = {.sequence_number = next_sequence_number_++,
                          .request_id = request_id},
      .body = body,
  };
  auto status = co_await transport_.WriteFrame(
      EncodeSecureConversationMessage(message));
  opened_ = false;
  co_return status;
}

std::uint32_t OpcUaBinaryClientSecureChannel::NextRequestId() {
  return next_request_id_++;
}

}  // namespace opcua
