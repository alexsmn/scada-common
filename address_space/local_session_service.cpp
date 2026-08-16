#include "address_space/local_session_service.h"

namespace scada {

LocalSessionService::LocalSessionService() = default;
LocalSessionService::~LocalSessionService() = default;

Awaitable<void> LocalSessionService::Connect(SessionConnectParams /*params*/) {
  co_return;
}

Awaitable<void> LocalSessionService::Reconnect() {
  co_return;
}

Awaitable<void> LocalSessionService::Disconnect() {
  co_return;
}

bool LocalSessionService::IsConnected(scada::Duration* ping_delay) const {
  // There is no round trip to measure, but the out-parameter still has to be
  // written: `scada::Duration` default-initializes to an uninitialized rep, so
  // a caller that reads it back gets garbage. See SessionService::IsConnected.
  if (ping_delay)
    *ping_delay = scada::Duration::zero();
  return true;
}

void LocalSessionService::SetUserId(NodeId user_id) {
  user_id_ = std::move(user_id);
}

NodeId LocalSessionService::GetUserId() const {
  return user_id_;
}

std::uint32_t LocalSessionService::GetAccessRights() const {
  return AccessRightBit(AccessRight::kConfigure) |
         AccessRightBit(AccessRight::kControl);
}

std::string LocalSessionService::GetHostName() const {
  return "local";
}

bool LocalSessionService::IsScada() const {
  return true;
}

boost::signals2::scoped_connection
LocalSessionService::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& /*callback*/) {
  return {};
}

SessionDebugger* LocalSessionService::GetSessionDebugger() {
  return nullptr;
}

}  // namespace scada
