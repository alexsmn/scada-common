#include "opcua_ws/opcua_ws_session_manager.h"

#include "scada/status_or.h"

#include <algorithm>
#include <utility>

namespace opcua_ws {

namespace {

scada::Status SessionMissingStatus() {
  return scada::StatusCode::Bad_SessionIsLoggedOff;
}

}  // namespace

OpcUaWsSessionManager::OpcUaWsSessionManager(
    OpcUaWsSessionManagerContext&& context)
    : OpcUaWsSessionManagerContext{std::move(context)} {}

Awaitable<OpcUaWsCreateSessionResponse> OpcUaWsSessionManager::CreateSession(
    OpcUaWsCreateSessionRequest request) {
  PruneExpiredSessions();

  const auto revised_timeout = ReviseTimeout(request.requested_timeout);
  const auto session_id = MakeSessionId();
  const auto authentication_token = MakeAuthenticationToken();
  const auto nonce_seed = next_token_id_ - 1;

  SessionState session{
      .session_id = session_id,
      .authentication_token = authentication_token,
      .server_nonce = MakeServerNonce(nonce_seed),
      .revised_timeout = revised_timeout,
      .expires_at = Now() + revised_timeout,
  };

  auto server_nonce = session.server_nonce;
  sessions_.insert_or_assign(authentication_token, std::move(session));

  co_return OpcUaWsCreateSessionResponse{
      .status = scada::StatusCode::Good,
      .session_id = session_id,
      .authentication_token = authentication_token,
      .server_nonce = std::move(server_nonce),
      .revised_timeout = revised_timeout,
  };
}

Awaitable<OpcUaWsActivateSessionResponse> OpcUaWsSessionManager::ActivateSession(
    OpcUaWsActivateSessionRequest request) {
  PruneExpiredSessions();

  auto session_it = sessions_.find(request.authentication_token);
  if (session_it == sessions_.end())
    co_return OpcUaWsActivateSessionResponse{SessionMissingStatus()};
  // cppcheck-suppress derefInvalidIteratorRedundantCheck
  auto& session = session_it->second;
  if (session.session_id != request.session_id)
    co_return OpcUaWsActivateSessionResponse{SessionMissingStatus()};

  if (session.activated) {
    session.attached = true;
    session.expires_at = Now() + session.revised_timeout;
    co_return OpcUaWsActivateSessionResponse{
        .status = scada::StatusCode::Good,
        .service_context = session.service_context,
        .authentication_result = session.authentication_result,
        .resumed = true,
    };
  }

  std::optional<scada::AuthenticationResult> auth_result;
  if (!request.allow_anonymous) {
    if (!request.user_name.has_value() || !request.password.has_value()) {
      co_return OpcUaWsActivateSessionResponse{
          scada::StatusCode::Bad_WrongLoginCredentials};
    }

    auto auth = co_await authenticator(std::move(*request.user_name),
                                       std::move(*request.password));
    if (!auth.ok())
      co_return OpcUaWsActivateSessionResponse{auth.status()};

    auth_result = *auth;
    if (!auth_result->multi_sessions) {
      if (HasSessionForUser(auth_result->user_id)) {
        if (!request.delete_existing) {
          co_return OpcUaWsActivateSessionResponse{
              scada::StatusCode::Bad_UserIsAlreadyLoggedOn};
        }
        [[maybe_unused]] const auto removed =
            RemoveSessionByUser(auth_result->user_id);
      }
    }
  }

  session_it = sessions_.find(request.authentication_token);
  if (session_it == sessions_.end())
    co_return OpcUaWsActivateSessionResponse{SessionMissingStatus()};
  // cppcheck-suppress derefInvalidIteratorRedundantCheck
  auto& refreshed_session = session_it->second;
  if (refreshed_session.session_id != request.session_id)
    co_return OpcUaWsActivateSessionResponse{SessionMissingStatus()};

  if (auth_result.has_value()) {
    refreshed_session.authentication_result = auth_result;
    refreshed_session.service_context =
        scada::ServiceContext{}.with_user_id(auth_result->user_id);
  } else {
    refreshed_session.service_context = scada::ServiceContext{};
  }
  refreshed_session.activated = true;
  refreshed_session.attached = true;
  refreshed_session.expires_at = Now() + refreshed_session.revised_timeout;

  co_return OpcUaWsActivateSessionResponse{
      .status = scada::StatusCode::Good,
      .service_context = refreshed_session.service_context,
      .authentication_result = refreshed_session.authentication_result,
      .resumed = false,
  };
}

OpcUaWsCloseSessionResponse OpcUaWsSessionManager::CloseSession(
    OpcUaWsCloseSessionRequest request) {
  auto* session = FindSessionState(request.authentication_token);
  if (!session || session->session_id != request.session_id)
    return {.status = SessionMissingStatus()};

  RemoveSessionByToken(request.authentication_token);
  return {.status = scada::StatusCode::Good};
}

void OpcUaWsSessionManager::DetachSession(
    const scada::NodeId& authentication_token) {
  if (auto* session = FindSessionState(authentication_token))
    session->attached = false;
}

void OpcUaWsSessionManager::PruneExpiredSessions() {
  const auto now_time = Now();
  std::erase_if(sessions_, [now_time](const auto& entry) {
    return entry.second.expires_at <= now_time;
  });
}

std::optional<OpcUaWsSessionLookupResult> OpcUaWsSessionManager::FindSession(
    const scada::NodeId& authentication_token) const {
  const auto* session = FindSessionState(authentication_token);
  if (!session)
    return std::nullopt;

  return OpcUaWsSessionLookupResult{
      .session_id = session->session_id,
      .authentication_token = session->authentication_token,
      .service_context = session->service_context,
      .authentication_result = session->authentication_result,
      .attached = session->attached,
      .activated = session->activated,
  };
}

base::TimeDelta OpcUaWsSessionManager::ReviseTimeout(
    base::TimeDelta requested) const {
  if (requested.is_zero())
    return default_timeout;
  return std::clamp(requested, min_timeout, max_timeout);
}

scada::NodeId OpcUaWsSessionManager::MakeSessionId() {
  return {next_session_id_++, session_namespace_index};
}

scada::NodeId OpcUaWsSessionManager::MakeAuthenticationToken() {
  return {next_token_id_++, token_namespace_index};
}

scada::ByteString OpcUaWsSessionManager::MakeServerNonce(
    scada::UInt32 seed) const {
  return scada::ByteString{
      static_cast<char>(seed & 0xff),
      static_cast<char>((seed >> 8) & 0xff),
      static_cast<char>((seed >> 16) & 0xff),
      static_cast<char>((seed >> 24) & 0xff),
      static_cast<char>(0xa5),
      static_cast<char>(0x5a),
      static_cast<char>(session_namespace_index & 0xff),
      static_cast<char>(token_namespace_index & 0xff),
  };
}

OpcUaWsSessionManager::SessionState* OpcUaWsSessionManager::FindSessionState(
    const scada::NodeId& authentication_token) {
  auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? &it->second : nullptr;
}

const OpcUaWsSessionManager::SessionState*
OpcUaWsSessionManager::FindSessionState(
    const scada::NodeId& authentication_token) const {
  auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? &it->second : nullptr;
}

bool OpcUaWsSessionManager::RemoveSessionByUser(const scada::NodeId& user_id) {
  auto it = std::find_if(sessions_.begin(), sessions_.end(),
                         [&user_id](const auto& entry) {
                           return entry.second.authentication_result.has_value() &&
                                  entry.second.authentication_result->user_id ==
                                      user_id;
                         });
  if (it == sessions_.end())
    return false;

  sessions_.erase(it);
  return true;
}

bool OpcUaWsSessionManager::HasSessionForUser(
    const scada::NodeId& user_id) const {
  return std::any_of(sessions_.begin(), sessions_.end(),
                     [&user_id](const auto& entry) {
                       return entry.second.authentication_result.has_value() &&
                              entry.second.authentication_result->user_id ==
                                  user_id;
                     });
}

void OpcUaWsSessionManager::RemoveSessionByToken(
    const scada::NodeId& authentication_token) {
  sessions_.erase(authentication_token);
}

}  // namespace opcua_ws
