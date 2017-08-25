#pragma once

namespace protocol {
class Message;
}

class Connection {
 public:
  virtual ~Connection() {}

  virtual void Send(protocol::Message& message) = 0;
  virtual void OnSessionDeleted() = 0;
};
