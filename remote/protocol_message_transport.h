#pragma once

#include <memory>
#include <string>

#include "net/transport.h"

class ProtocolMessageTransport : public net::Transport,
                                 private net::Transport::Delegate {
 public:
  explicit ProtocolMessageTransport(std::unique_ptr<net::Transport> transport);
  ~ProtocolMessageTransport();

  virtual net::Error Open() override;
  virtual void Close() override;
  virtual int Read(void* data, size_t len) override;
  virtual int Write(const void* data, size_t len) override;
  virtual std::string GetName() const override;
  virtual bool IsMessageOriented() const override { return true; }
  virtual bool IsConnected() const override { return transport_->IsConnected(); }
  virtual bool IsActive() const override { return transport_->IsActive(); }

 private:
  // net::Transport::Delegate
  virtual void OnTransportOpened() override;
  virtual void OnTransportClosed(net::Error error) override;
  virtual void OnTransportDataReceived() override;

  std::unique_ptr<net::Transport> transport_;

  std::string incoming_message_;

  std::shared_ptr<bool> cancelation_;
};
