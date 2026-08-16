#pragma once

#include "scada/session_service.h"

namespace scada {

// Trivial in-memory SessionService. Reports `IsConnected() == true` with a
// zero ping delay, grants every access right, and completes all lifecycle
// coroutines immediately.
//
// Intended for tests, demos, and screenshot tooling where the session layer
// is not under test.
class LocalSessionService : public SessionService {
 public:
  LocalSessionService();
  ~LocalSessionService() override;

  // Names the signed-in user, so consumers that resolve `GetUserId()` against
  // an address space render a real identity. Without it the id is null and a
  // lookup finds nothing — which is how the client's status strip came to show
  // a bare role with no user name under the screenshot fixture. Set it before
  // anything subscribes; there is no state-changed signal to raise.
  void SetUserId(NodeId user_id);

  Awaitable<void> Connect(SessionConnectParams params) override;
  Awaitable<void> Reconnect() override;
  Awaitable<void> Disconnect() override;

  bool IsConnected(scada::Duration* ping_delay = nullptr) const override;

  NodeId GetUserId() const override;
  std::uint32_t GetAccessRights() const override;

  std::string GetHostName() const override;
  bool IsScada() const override;

  boost::signals2::scoped_connection SubscribeSessionStateChanged(
      const SessionStateChangedCallback& callback) override;

  SessionDebugger* GetSessionDebugger() override;

 private:
  NodeId user_id_;
};

}  // namespace scada
