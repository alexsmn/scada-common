#pragma once

#include "base/memory/weak_ptr.h"
#include "core/history_types.h"

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
  HistoryStub(MessageSender& sender, scada::HistoryService& history_service, std::shared_ptr<Logger> logger);

  void OnRequestReceived(const protocol::Request& request);

 private:
  Logger& logger() { return *logger_; }

  void OnQueryEventsCompleted(unsigned request_id,
                              scada::QueryEventsResults results);

  MessageSender& sender_;
  scada::HistoryService& history_service_;
  std::shared_ptr<Logger> logger_;

  base::WeakPtrFactory<HistoryStub> weak_factory_;
};
