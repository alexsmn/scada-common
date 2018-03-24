#pragma once

#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/view_service_proxy.h"
#include "remote/view_service_stub.h"

namespace test {

class TestMessageSender : public MessageSender {
 public:
  using SendHandler = std::function<void(protocol::Message& message)>;
  using RequestHandler = std::function<void(protocol::Request& request, ResponseHandler response_handler)>;

  TestMessageSender(SendHandler send_handler, RequestHandler request_handler)
      : send_handler_{std::move(send_handler)},
        request_handler_{std::move(request_handler)} {
  }

  virtual void Send(protocol::Message& message) override {
    send_handler_(message);
  }

  virtual void Request(protocol::Request& request, ResponseHandler response_handler) override {
    request_handler_(request, std::move(response_handler));
  }

  const SendHandler send_handler_;
  const RequestHandler request_handler_;
};

class ViewServiceChannel {
 public:
  explicit ViewServiceChannel(scada::ViewService& view_service);

  scada::ViewService& proxy() { return proxy_; }

 private:
  scada::ViewService& service_;
  const std::shared_ptr<Logger> logger_ = std::make_shared<NullLogger>();
  ViewServiceProxy proxy_;
  TestMessageSender stub_sender_;
  ViewServiceStub stub_;
  TestMessageSender proxy_sender_;

  std::map<unsigned /*request_id*/, MessageSender::ResponseHandler> proxy_requests_;
};

inline ViewServiceChannel::ViewServiceChannel(scada::ViewService& view_service)
    : service_{view_service},
      proxy_{logger_},
      stub_sender_{
          [this](protocol::Message& message) {
            for (auto& response : message.responses()) {
              auto i = proxy_requests_.find(response.request_id());
              if (i != proxy_requests_.end())
                i->second(std::move(response));
            }
            for (auto& notification : message.notifications())
              proxy_.OnNotification(notification);
          },
          [this](protocol::Request& request, MessageSender::ResponseHandler response_handler) {
            assert(false);
          },
      },
      stub_{stub_sender_, service_, logger_},
      proxy_sender_{
          [this](protocol::Message& message) {
            assert(false);
          },
          [this](protocol::Request& request, MessageSender::ResponseHandler response_handler) {
            proxy_requests_.emplace(request.request_id(), std::move(response_handler));
            stub_.OnRequestReceived(request);
          },
      } {
  proxy_.OnChannelOpened(proxy_sender_);
}

} // namespace test
