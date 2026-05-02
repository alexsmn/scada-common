#pragma once

#include "base/any_executor.h"
#include "scada/data_services.h"
#include "scada/services.h"

#include <memory>

class EventFetcher;
class Logger;

namespace scada {
class HistoryService;
class MethodService;
class SessionService;
}  // namespace scada

struct EventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  AnyExecutor executor_;
  std::shared_ptr<const Logger> logger_;

  DataServices data_services_;

  // TODO: Switch to `scada::client`.
  scada::services services_;
};

struct CoroutineEventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  AnyExecutor executor_;
  std::shared_ptr<const Logger> logger_;

  // TODO: Switch to `scada::client`.
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
};
