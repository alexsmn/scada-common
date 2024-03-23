#pragma once

#include "scada/services.h"

#include <memory>

class EventFetcher;
class Executor;
class Logger;

struct EventFetcherBuilder {
  std::shared_ptr<EventFetcher> Build();

  std::shared_ptr<Executor> executor_;
  std::shared_ptr<const Logger> logger_;

  // TODO: Switch to `scada::client`.
  scada::services services_;
};
