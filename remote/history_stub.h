#pragma once

#include "base/memory/weak_ptr.h"
#include "core/history_types.h"

#include <map>

namespace boost::asio {
class io_context;
}

namespace protocol {
class Request;
}

namespace scada {
class HistoryService;
}

class Logger;
class MessageSender;

class HistoryStub {
 public:
  HistoryStub(scada::HistoryService& service,
              MessageSender& sender,
              boost::asio::io_context& io_context,
              std::shared_ptr<Logger> logger);
  ~HistoryStub();

  void OnRequestReceived(const protocol::Request& request);

 private:
  void OnHistoryReadRaw(const protocol::Request& request);
  void OnHistoryReadEvents(const protocol::Request& request);

  scada::HistoryService& service_;
  MessageSender& sender_;
  boost::asio::io_context& io_context_;
  std::shared_ptr<Logger> logger_;

  std::map<scada::ByteString, scada::HistoryReadRawDetails>
      continuation_points_;

  base::WeakPtrFactory<HistoryStub> weak_factory_{this};
};
