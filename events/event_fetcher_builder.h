#pragma once

#include "base/any_executor.h"
#include "scada/data_services.h"
#include "scada/services.h"

#include <memory>

class EventFetcher;
class Logger;

namespace scada {
class CoroutineHistoryService;
class CoroutineMethodService;
class CoroutineSessionService;
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
  scada::CoroutineHistoryService& history_service_;
  scada::CoroutineMethodService& method_service_;
  scada::CoroutineSessionService& session_service_;
};
