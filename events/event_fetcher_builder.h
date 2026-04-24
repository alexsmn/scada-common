#pragma once

#include "base/any_executor.h"
#include "scada/services.h"

#include <memory>

class EventFetcher;
class Logger;

struct EventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  AnyExecutor executor_;
  std::shared_ptr<const Logger> logger_;

  // TODO: Switch to `scada::client`.
  scada::services services_;
};
