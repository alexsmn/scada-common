#include "opcua/opcua_server_session_manager.h"

#include "base/boost_log.h"
#include "scada/status_or.h"

#include <algorithm>
#include <utility>

namespace opcua {

namespace {

BoostLogger logger_{LOG_NAME("OpcUaSessionManager")};

scada::Status SessionMissingStatus() {
  return scada::StatusCode::Bad_SessionIsLoggedOff;
}

}  // namespace

OpcUaSessionManager::OpcUaSessionManager(OpcUaSessionManagerContext&& context)
    : OpcUaSessionManagerContext{std::move(context)} {}

Awaitable<OpcUaCreateSessionResponse> OpcUaSessionManager::CreateSession(
    OpcUaCreateSessionRequest request) {
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

  LOG_INFO(logger_) << "OPC UA session created"
                    << LOG_TAG("SessionId", session_id.ToString())
                    << LOG_TAG("AuthenticationToken",
                               authentication_token.ToString())
                    << LOG_TAG("RequestedTimeoutMs",
                               request.requested_timeout.InMilliseconds())
                    << LOG_TAG("RevisedTimeoutMs",
                               revised_timeout.InMilliseconds());

  co_return OpcUaCreateSessionResponse{
      .status = scada::StatusCode::Good,
      .session_id = session_id,
      .authentication_token = authentication_token,
      .server_nonce = std::move(server_nonce),
      .revised_timeout = revised_timeout,
  };
}

Awaitable<OpcUaActivateSessionResponse> OpcUaSessionManager::ActivateSession(
    OpcUaActivateSessionRequest request) {
  PruneExpiredSessions();

  auto session_it = sessions_.find(request.authentication_token);
  if (session_it == sessions_.end())
    co_return OpcUaActivateSessionResponse{SessionMissingStatus()};
  // cppcheck-suppress derefInvalidIteratorRedundantCheck
  auto& session = session_it->second;
  if (session.session_id != request.session_id) {
    LOG_WARNING(logger_) << "OPC UA session activation failed"
                         << LOG_TAG("Reason", "SessionMismatch")
                         << LOG_TAG("SessionId", request.session_id.ToString())
                         << LOG_TAG("AuthenticationToken",
                                    request.authentication_token.ToString());
    co_return OpcUaActivateSessionResponse{SessionMissingStatus()};
  }

  if (session.activated) {
    session.attached = true;
    session.expires_at = Now() + session.revised_timeout;
    LOG_INFO(logger_) << "OPC UA session resumed"
                      << LOG_TAG("SessionId", session.session_id.ToString())
                      << LOG_TAG("AuthenticationToken",
                                 session.authentication_token.ToString())
                      << LOG_TAG("Attached", session.attached);
    co_return OpcUaActivateSessionResponse{
        .status = scada::StatusCode::Good,
        .service_context = session.service_context,
        .authentication_result = session.authentication_result,
        .resumed = true,
    };
  }

  std::optional<scada::AuthenticationResult> auth_result;
  if (!request.allow_anonymous) {
    if (!request.user_name.has_value() || !request.password.has_value()) {
      LOG_WARNING(logger_) << "OPC UA session activation failed"
                           << LOG_TAG("Reason", "MissingCredentials")
                           << LOG_TAG("SessionId", request.session_id.ToString())
                           << LOG_TAG("AuthenticationToken",
                                      request.authentication_token.ToString());
      co_return OpcUaActivateSessionResponse{
          scada::StatusCode::Bad_WrongLoginCredentials};
    }

    auto auth = co_await authenticator->Authenticate(
        std::move(*request.user_name), std::move(*request.password));
    if (!auth.ok()) {
      LOG_WARNING(logger_) << "OPC UA session activation failed"
                           << LOG_TAG("Reason", "AuthenticationFailed")
                           << LOG_TAG("SessionId", request.session_id.ToString())
                           << LOG_TAG("AuthenticationToken",
                                      request.authentication_token.ToString());
      co_return OpcUaActivateSessionResponse{auth.status()};
    }

    auth_result = *auth;
    if (!auth_result->multi_sessions) {
      if (HasSessionForUser(auth_result->user_id)) {
        if (!request.delete_existing) {
          LOG_WARNING(logger_) << "OPC UA session activation failed"
                               << LOG_TAG("Reason", "UserAlreadyLoggedOn")
                               << LOG_TAG("SessionId", request.session_id.ToString())
                               << LOG_TAG("AuthenticationToken",
                                          request.authentication_token.ToString())
                               << LOG_TAG("UserId",
                                          auth_result->user_id.ToString());
          co_return OpcUaActivateSessionResponse{
              scada::StatusCode::Bad_UserIsAlreadyLoggedOn};
        }
        [[maybe_unused]] const auto removed =
            RemoveSessionByUser(auth_result->user_id);
      }
    }
  }

  session_it = sessions_.find(request.authentication_token);
  if (session_it == sessions_.end())
    co_return OpcUaActivateSessionResponse{SessionMissingStatus()};
  // cppcheck-suppress derefInvalidIteratorRedundantCheck
  auto& refreshed_session = session_it->second;
  if (refreshed_session.session_id != request.session_id) {
    LOG_WARNING(logger_) << "OPC UA session activation failed"
                         << LOG_TAG("Reason", "SessionMismatchAfterAuth")
                         << LOG_TAG("SessionId", request.session_id.ToString())
                         << LOG_TAG("AuthenticationToken",
                                    request.authentication_token.ToString());
    co_return OpcUaActivateSessionResponse{SessionMissingStatus()};
  }

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

  if (auth_result.has_value()) {
    LOG_INFO(logger_) << "OPC UA session activated"
                      << LOG_TAG("SessionId",
                                 refreshed_session.session_id.ToString())
                      << LOG_TAG("AuthenticationToken",
                                 refreshed_session.authentication_token.ToString())
                      << LOG_TAG("UserId",
                                 auth_result->user_id.ToString())
                      << LOG_TAG("MultiSessions",
                                 auth_result->multi_sessions);
  } else {
    LOG_INFO(logger_) << "OPC UA anonymous session activated"
                      << LOG_TAG("SessionId",
                                 refreshed_session.session_id.ToString())
                      << LOG_TAG("AuthenticationToken",
                                 refreshed_session.authentication_token.ToString());
  }

  co_return OpcUaActivateSessionResponse{
      .status = scada::StatusCode::Good,
      .service_context = refreshed_session.service_context,
      .authentication_result = refreshed_session.authentication_result,
      .resumed = false,
  };
}

OpcUaCloseSessionResponse OpcUaSessionManager::CloseSession(
    OpcUaCloseSessionRequest request) {
  auto* session = FindSessionState(request.authentication_token);
  if (!session || session->session_id != request.session_id) {
    LOG_WARNING(logger_) << "OPC UA session close failed"
                         << LOG_TAG("Reason", "SessionMissing")
                         << LOG_TAG("SessionId", request.session_id.ToString())
                         << LOG_TAG("AuthenticationToken",
                                    request.authentication_token.ToString());
    return {.status = SessionMissingStatus()};
  }

  LOG_INFO(logger_) << "OPC UA session closed"
                    << LOG_TAG("SessionId", session->session_id.ToString())
                    << LOG_TAG("AuthenticationToken",
                               session->authentication_token.ToString())
                    << LOG_TAG("Attached", session->attached);

  RemoveSessionByToken(request.authentication_token);
  return {.status = scada::StatusCode::Good};
}

void OpcUaSessionManager::DetachSession(
    const scada::NodeId& authentication_token) {
  if (auto* session = FindSessionState(authentication_token)) {
    session->attached = false;
    LOG_INFO(logger_) << "OPC UA session detached"
                      << LOG_TAG("SessionId", session->session_id.ToString())
                      << LOG_TAG("AuthenticationToken",
                                 session->authentication_token.ToString());
  }
}

void OpcUaSessionManager::PruneExpiredSessions() {
  const auto now_time = Now();
  std::erase_if(sessions_, [now_time](const auto& entry) {
    const auto expired = entry.second.expires_at <= now_time;
    if (expired) {
      LOG_INFO(logger_) << "OPC UA session expired"
                        << LOG_TAG("SessionId", entry.second.session_id.ToString())
                        << LOG_TAG("AuthenticationToken",
                                   entry.second.authentication_token.ToString())
                        << LOG_TAG("Activated", entry.second.activated)
                        << LOG_TAG("Attached", entry.second.attached);
    }
    return expired;
  });
}

std::optional<OpcUaSessionLookupResult> OpcUaSessionManager::FindSession(
    const scada::NodeId& authentication_token) const {
  const auto* session = FindSessionState(authentication_token);
  if (!session)
    return std::nullopt;

  return OpcUaSessionLookupResult{
      .session_id = session->session_id,
      .authentication_token = session->authentication_token,
      .service_context = session->service_context,
      .authentication_result = session->authentication_result,
      .attached = session->attached,
      .activated = session->activated,
  };
}

base::TimeDelta OpcUaSessionManager::ReviseTimeout(
    base::TimeDelta requested) const {
  if (requested.is_zero())
    return default_timeout;
  return std::clamp(requested, min_timeout, max_timeout);
}

scada::NodeId OpcUaSessionManager::MakeSessionId() {
  return {next_session_id_++, session_namespace_index};
}

scada::NodeId OpcUaSessionManager::MakeAuthenticationToken() {
  return {next_token_id_++, token_namespace_index};
}

scada::ByteString OpcUaSessionManager::MakeServerNonce(
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

OpcUaSessionManager::SessionState* OpcUaSessionManager::FindSessionState(
    const scada::NodeId& authentication_token) {
  auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? &it->second : nullptr;
}

const OpcUaSessionManager::SessionState*
OpcUaSessionManager::FindSessionState(
    const scada::NodeId& authentication_token) const {
  auto it = sessions_.find(authentication_token);
  return it != sessions_.end() ? &it->second : nullptr;
}

bool OpcUaSessionManager::RemoveSessionByUser(const scada::NodeId& user_id) {
  auto it = std::find_if(sessions_.begin(), sessions_.end(),
                         [&user_id](const auto& entry) {
                           return entry.second.authentication_result.has_value() &&
                                  entry.second.authentication_result->user_id ==
                                      user_id;
                         });
  if (it == sessions_.end())
    return false;

  LOG_INFO(logger_) << "OPC UA session removed for single-session user"
                    << LOG_TAG("SessionId", it->second.session_id.ToString())
                    << LOG_TAG("AuthenticationToken",
                               it->second.authentication_token.ToString())
                    << LOG_TAG("UserId", user_id.ToString());
  sessions_.erase(it);
  return true;
}

bool OpcUaSessionManager::HasSessionForUser(
    const scada::NodeId& user_id) const {
  return std::any_of(sessions_.begin(), sessions_.end(),
                     [&user_id](const auto& entry) {
                       return entry.second.authentication_result.has_value() &&
                              entry.second.authentication_result->user_id ==
                                  user_id;
                     });
}

void OpcUaSessionManager::RemoveSessionByToken(
    const scada::NodeId& authentication_token) {
  if (auto* session = FindSessionState(authentication_token)) {
    LOG_INFO(logger_) << "OPC UA session forgotten"
                      << LOG_TAG("SessionId", session->session_id.ToString())
                      << LOG_TAG("AuthenticationToken",
                                 session->authentication_token.ToString());
  }
  sessions_.erase(authentication_token);
}

}  // namespace opcua
