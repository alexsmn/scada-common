#pragma once

#include "base/memory/weak_ptr.h"
#include "core/history_types.h"

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

  void OnRequestReceived(const protocol::Request& request);

 private:
  void OnHistoryReadCompleted(unsigned request_id,
                              const scada::Status& status,
                              scada::QueryValuesResults values,
                              scada::QueryEventsResults events);

  scada::HistoryService& service_;
  MessageSender& sender_;
  boost::asio::io_context& io_context_;
  std::shared_ptr<Logger> logger_;

  base::WeakPtrFactory<HistoryStub> weak_factory_{this};
};
