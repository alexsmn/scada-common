#pragma once

#include <functional>

namespace protocol {
class Message;
class Request;
class Response;
}

class MessageSender {
 public:
  virtual ~MessageSender() {}

  virtual void Send(protocol::Message& message) = 0;

  typedef std::function<void(const protocol::Response&)> ResponseHandler;
  virtual void Request(protocol::Request& request, ResponseHandler response_handler) = 0;
};
