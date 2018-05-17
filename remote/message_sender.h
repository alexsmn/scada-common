#pragma once

#include <functional>

namespace protocol {
class Message;
class Request;
class Response;
}  // namespace protocol

class MessageSender {
 public:
  virtual ~MessageSender() {}

  virtual void Send(protocol::Message& message) = 0;

  using ResponseHandler = std::function<void(protocol::Response&&)>;
  virtual void Request(protocol::Request& request,
                       ResponseHandler response_handler) = 0;
};
