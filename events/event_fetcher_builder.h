#pragma once

#include <memory>

namespace scada {
class MonitoredItemService;
class HistoryService;
class MethodService;
class SessionService;
}  // namespace scada

class EventFetcher;
class Executor;
class Logger;

struct EventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  std::shared_ptr<Executor> executor_;
  std::shared_ptr<const Logger> logger_;

  // TODO: Switch to `scada::client`.
  scada::MonitoredItemService& monitored_item_service_;
  scada::HistoryService& history_service_;
  scada::MethodService& method_service_;
  scada::SessionService& session_service_;
};
