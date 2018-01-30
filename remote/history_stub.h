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
  HistoryStub(scada::HistoryService& service,
              MessageSender& sender,
              std::shared_ptr<Logger> logger);

  void OnRequestReceived(const protocol::Request& request);

 private:
  scada::HistoryService& service_;
  MessageSender& sender_;
  std::shared_ptr<Logger> logger_;

  base::WeakPtrFactory<HistoryStub> weak_factory_{this};
};
