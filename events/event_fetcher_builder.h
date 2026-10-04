#pragma once

#include "base/any_executor.h"
#include "base/boost_log.h"
#include "events/event_ack_queue.h"
#include "scada/data_services.h"
#include "scada/services.h"

#include <memory>

class EventFetcher;

namespace scada {
class HistoryService;
class MethodService;
class SessionService;
}  // namespace scada

struct EventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  AnyExecutor executor_;
  std::shared_ptr<BoostLogger> logger_;

  DataServices data_services_;

  // TODO: Switch to `scada::client`.
  scada::services services_;

  // Optional: told when the Server refuses an acknowledgement.
  EventAckFailedHandler ack_failed_handler_ = {};
};

struct CoroutineEventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  AnyExecutor executor_;
  std::shared_ptr<BoostLogger> logger_;

  // TODO: Switch to `scada::client`.
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
};
