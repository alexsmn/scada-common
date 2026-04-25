#include "opcua/binary/tcp_connection.h"

#include <algorithm>

namespace opcua::binary {
namespace {

std::vector<char> SubspanToVector(const std::vector<char>& bytes,
                                  std::size_t offset,
                                  std::size_t size) {
  return {bytes.begin() + static_cast<std::ptrdiff_t>(offset),
          bytes.begin() + static_cast<std::ptrdiff_t>(offset + size)};
}

}  // namespace

TcpConnection::TcpConnection(
    TcpConnectionContext&& context)
    : TcpConnectionContext{std::move(context)}, secure_channel_{} {}

Awaitable<void> TcpConnection::Run() {
  [[maybe_unused]] auto open_result = co_await transport.open();
  transport::WriteQueue write_queue{transport};
  std::vector<char> read_buffer(read_buffer_size);
  std::vector<char> pending_bytes;

  for (;;) {
    auto read_result = co_await transport.read(read_buffer);
    if (!read_result.ok() || *read_result == 0) {
      break;
    }

    pending_bytes.insert(pending_bytes.end(), read_buffer.begin(),
                         read_buffer.begin() +
                             static_cast<std::ptrdiff_t>(*read_result));
    if (!(co_await ProcessBufferedFrames(write_queue, pending_bytes))) {
      break;
    }
  }

  [[maybe_unused]] auto close_result = co_await transport.close();
}

Awaitable<bool> TcpConnection::ProcessBufferedFrames(
    transport::WriteQueue& write_queue,
    std::vector<char>& pending_bytes) {
  for (;;) {
    if (pending_bytes.size() < 8) {
      co_return true;
    }

    const auto header = DecodeFrameHeader(
        std::vector<char>{pending_bytes.begin(), pending_bytes.begin() + 8});
    if (!header.has_value()) {
      co_return co_await WriteErrorAndClose(
          write_queue, scada::StatusCode::Bad, "Invalid UA TCP frame header");
    }
    if (header->message_size > max_frame_size) {
      co_return co_await WriteErrorAndClose(
          write_queue, scada::StatusCode::Bad, "UA TCP frame too large");
    }
    if (pending_bytes.size() < header->message_size) {
      co_return true;
    }

    auto frame = SubspanToVector(pending_bytes, 0, header->message_size);
    pending_bytes.erase(
        pending_bytes.begin(),
        pending_bytes.begin() + static_cast<std::ptrdiff_t>(header->message_size));
    if (!(co_await ProcessFrame(write_queue, frame))) {
      co_return false;
    }
  }
}

Awaitable<bool> TcpConnection::ProcessFrame(
    transport::WriteQueue& write_queue,
    const std::vector<char>& frame) {
  const auto header = DecodeFrameHeader(frame);
  if (!header.has_value()) {
    co_return co_await WriteErrorAndClose(
        write_queue, scada::StatusCode::Bad, "Invalid UA TCP frame header");
  }

  switch (header->message_type) {
    case MessageType::Hello: {
      if (hello_received_) {
        co_return co_await WriteErrorAndClose(
            write_queue, scada::StatusCode::Bad,
            "Hello may only be sent once per connection");
      }

      const auto hello = DecodeHelloMessage(frame);
      if (!hello.has_value()) {
        co_return co_await WriteErrorAndClose(
            write_queue, scada::StatusCode::Bad, "Malformed Hello message");
      }

      const auto negotiated = NegotiateHello(*hello, limits);
      if (negotiated.error.has_value()) {
        const auto encoded = EncodeErrorMessage(*negotiated.error);
        [[maybe_unused]] auto write_result =
            co_await write_queue.Write({encoded.data(), encoded.size()});
        co_return false;
      }

      hello_received_ = true;
      const auto encoded =
          EncodeAcknowledgeMessage(*negotiated.acknowledge);
      [[maybe_unused]] auto write_result =
          co_await write_queue.Write({encoded.data(), encoded.size()});
      co_return true;
    }

    case MessageType::SecureOpen:
    case MessageType::SecureMessage:
    case MessageType::SecureClose: {
      if (!hello_received_) {
        co_return co_await WriteErrorAndClose(
            write_queue, scada::StatusCode::Bad,
            "SecureChannel traffic received before Hello/Acknowledge");
      }

      auto result = co_await secure_channel_.HandleFrame(frame);
      if (result.outbound_frame.has_value() && !result.outbound_frame->empty()) {
        [[maybe_unused]] auto write_result =
            co_await write_queue.Write(
                {result.outbound_frame->data(), result.outbound_frame->size()});
      }
      if (result.close_transport) {
        co_return false;
      }
      if (result.service_payload.has_value()) {
        auto outbound_payload =
            co_await on_secure_frame(std::move(*result.service_payload));
        if (outbound_payload.has_value() && !outbound_payload->empty() &&
            result.request_id.has_value()) {
          auto outbound_frame = secure_channel_.BuildServiceResponse(
              *result.request_id, std::move(*outbound_payload));
          [[maybe_unused]] auto write_result = co_await write_queue.Write(
              {outbound_frame.data(), outbound_frame.size()});
        }
      }
      co_return true;
    }

    case MessageType::Acknowledge:
    case MessageType::Error:
    case MessageType::ReverseHello:
      co_return co_await WriteErrorAndClose(
          write_queue, scada::StatusCode::Bad,
          "Unexpected UA TCP message type for server connection");
  }

  co_return false;
}

Awaitable<bool> TcpConnection::WriteErrorAndClose(
    transport::WriteQueue& write_queue,
    scada::Status error,
    std::string reason) {
  const auto encoded = EncodeErrorMessage(
      {.error = error, .reason = std::move(reason)});
  [[maybe_unused]] auto write_result =
      co_await write_queue.Write({encoded.data(), encoded.size()});
  co_return false;
}

}  // namespace opcua::binary
