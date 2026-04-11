#pragma once

#include "scada/session_service.h"

namespace scada {

// Trivial in-memory SessionService. Reports `IsConnected() == true`, grants
// every privilege, and resolves all lifecycle promises immediately.
//
// Intended for tests, demos, and screenshot tooling where the session layer
// is not under test.
class LocalSessionService : public SessionService {
 public:
  LocalSessionService();
  ~LocalSessionService() override;

  promise<void> Connect(const SessionConnectParams& params) override;
  promise<void> Reconnect() override;
  promise<void> Disconnect() override;

  bool IsConnected(base::TimeDelta* ping_delay = nullptr) const override;

  NodeId GetUserId() const override;
  bool HasPrivilege(Privilege privilege) const override;

  std::string GetHostName() const override;
  bool IsScada() const override;

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;

  SessionDebugger* GetSessionDebugger() override;
};

}  // namespace scada
