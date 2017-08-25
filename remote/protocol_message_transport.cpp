#include "remote/protocol_message_transport.h"

#include "net/transport_string.h"
#include "remote/protocol_buffer.h"

ProtocolMessageTransport::ProtocolMessageTransport(
    std::unique_ptr<net::Transport> transport)
    : transport_(std::move(transport)),
      cancelation_(std::make_shared<bool>(false)) {
  assert(!transport_->IsMessageOriented());
  transport_->set_delegate(this);
}

ProtocolMessageTransport::~ProtocolMessageTransport() {
}

net::Error ProtocolMessageTransport::Open() {
  return transport_->Open();
}

void ProtocolMessageTransport::Close() {
  if (transport_)
    transport_->Close();
}

int ProtocolMessageTransport::Read(void* data, size_t len) {
  return net::ERR_ABORTED;
}

int ProtocolMessageTransport::Write(const void* data, size_t len) {
  std::string message;
  protocol::PrependMessageSize(message);
  message.insert(message.end(), static_cast<const char*>(data),
                                static_cast<const char*>(data) + len);
  protocol::UpdateMessageSize(message);
  return transport_->Write(message.data(), message.size());
}

std::string ProtocolMessageTransport::GetName() const {
  return transport_->GetName();
}

void ProtocolMessageTransport::OnTransportOpened() {
  if (delegate_)
    delegate_->OnTransportOpened();
}

void ProtocolMessageTransport::OnTransportClosed(net::Error error) {
  if (delegate_)
    delegate_->OnTransportClosed(error);
}

void ProtocolMessageTransport::OnTransportDataReceived() {
  if (!delegate_)
    return;

  std::weak_ptr<bool> cancelation = cancelation_;
  while (!cancelation.expired()) {
    while (incoming_message_.size() <
           protocol::GetIncomingMessageSize(incoming_message_)) {
      auto original_size = incoming_message_.size();
      auto size = protocol::GetIncomingMessageSize(incoming_message_);
      incoming_message_.resize(size);
      auto read_count = transport_->Read(
          &incoming_message_[original_size], size - original_size);
      if (read_count < 0) {
        delegate_->OnTransportClosed(static_cast<net::Error>(read_count));
        return;
      }
      incoming_message_.resize(original_size + read_count);
      if (incoming_message_.size() < size)
        return;
    }

    auto incoming_message = std::move(incoming_message_);
    delegate_->OnTransportMessageReceived(
        protocol::GetMessagePayload(incoming_message),
        protocol::GetMessagePayloadSize(incoming_message));

    if (cancelation.expired())
      break;
  }
}
