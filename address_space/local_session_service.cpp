#include "address_space/local_session_service.h"

#include "base/promise.h"

namespace scada {

LocalSessionService::LocalSessionService() = default;
LocalSessionService::~LocalSessionService() = default;

promise<void> LocalSessionService::Connect(
    const SessionConnectParams& /*params*/) {
  return make_resolved_promise();
}

promise<void> LocalSessionService::Reconnect() {
  return make_resolved_promise();
}

promise<void> LocalSessionService::Disconnect() {
  return make_resolved_promise();
}

bool LocalSessionService::IsConnected(
    base::TimeDelta* /*ping_delay*/) const {
  return true;
}

NodeId LocalSessionService::GetUserId() const {
  return {};
}

bool LocalSessionService::HasPrivilege(Privilege /*privilege*/) const {
  return true;
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

LocalCoroutineSessionService::LocalCoroutineSessionService() = default;
LocalCoroutineSessionService::~LocalCoroutineSessionService() = default;

Awaitable<void> LocalCoroutineSessionService::Connect(
    SessionConnectParams /*params*/) {
  co_return;
}

Awaitable<void> LocalCoroutineSessionService::Reconnect() {
  co_return;
}

Awaitable<void> LocalCoroutineSessionService::Disconnect() {
  co_return;
}

bool LocalCoroutineSessionService::IsConnected(
    base::TimeDelta* /*ping_delay*/) const {
  return true;
}

NodeId LocalCoroutineSessionService::GetUserId() const {
  return {};
}

bool LocalCoroutineSessionService::HasPrivilege(Privilege /*privilege*/) const {
  return true;
}

std::string LocalCoroutineSessionService::GetHostName() const {
  return "local";
}

bool LocalCoroutineSessionService::IsScada() const {
  return true;
}

boost::signals2::scoped_connection
LocalCoroutineSessionService::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& /*callback*/) {
  return {};
}

SessionDebugger* LocalCoroutineSessionService::GetSessionDebugger() {
  return nullptr;
}

}  // namespace scada
