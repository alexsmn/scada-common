#pragma once

#include "core/status.h"

namespace scada {

class SessionStateObserver {
 public:
  virtual void OnSessionCreated() { }
  // Login incorrect, or server side close or fatal connection error.
  virtual void OnSessionDeleted(const Status& status) { }
};

} // namespace scada
