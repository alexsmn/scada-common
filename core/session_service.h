#pragma once

#include "core/node_id.h"
#include "core/status.h"

#include <functional>
#include <string>

namespace scada {

class SessionStateObserver;

class SessionService {
 public:
  virtual ~SessionService() {}

  typedef std::function<void(const Status&)> ConnectCallback;
  virtual void Connect(const std::string& connection_string, const std::string& username,
                       const std::string& password, bool allow_remote_logoff,
                       ConnectCallback callback) = 0;

  virtual bool IsConnected() const = 0;
  virtual bool IsAdministrator() const = 0;
  virtual bool IsScada() const = 0;

  virtual NodeId GetUserId() const = 0;
  virtual std::string GetHostName() const = 0;

  virtual void AddObserver(SessionStateObserver& observer) = 0;
  virtual void RemoveObserver(SessionStateObserver& observer) = 0;
};

} // namespace scada
