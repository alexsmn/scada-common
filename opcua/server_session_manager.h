#pragma once

#include "base/awaitable.h"
#include "base/time/time.h"
#include "scada/authentication.h"
#include "scada/service_context.h"
#include "scada/status.h"

#include <functional>
#include <optional>
#include <unordered_map>

namespace opcua {

struct CreateSessionRequest {
  base::TimeDelta requested_timeout = base::TimeDelta::FromMinutes(10);
};

struct CreateSessionResponse {
  scada::Status status{scada::StatusCode::Good};
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::ByteString server_nonce;
  base::TimeDelta revised_timeout;
};

struct ActivateSessionRequest {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  std::optional<scada::LocalizedText> user_name;
  std::optional<scada::LocalizedText> password;
  bool delete_existing = false;
  bool allow_anonymous = false;
};

struct ActivateSessionResponse {
  scada::Status status{scada::StatusCode::Good};
  scada::ServiceContext service_context;
  std::optional<scada::AuthenticationResult> authentication_result;
  bool resumed = false;
};

struct CloseSessionRequest {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
};

struct CloseSessionResponse {
  scada::Status status{scada::StatusCode::Good};
};

struct ServerSessionLookupResult {
  scada::NodeId session_id;
  scada::NodeId authentication_token;
  scada::ServiceContext service_context;
  std::optional<scada::AuthenticationResult> authentication_result;
  bool attached = false;
  bool activated = false;
};

struct ServerSessionManagerContext {
  std::shared_ptr<scada::CoroutineAuthenticator> authenticator;
  std::function<base::Time()> now = &base::Time::Now;
  base::TimeDelta default_timeout = base::TimeDelta::FromMinutes(10);
  base::TimeDelta min_timeout = base::TimeDelta::FromSeconds(30);
  base::TimeDelta max_timeout = base::TimeDelta::FromHours(1);
  scada::NamespaceIndex session_namespace_index = 2;
  scada::NamespaceIndex token_namespace_index = 3;
};

class ServerSessionManager : private ServerSessionManagerContext {
 public:
  explicit ServerSessionManager(ServerSessionManagerContext&& context);

  [[nodiscard]] Awaitable<CreateSessionResponse> CreateSession(
      CreateSessionRequest request = {});
  [[nodiscard]] Awaitable<ActivateSessionResponse> ActivateSession(
      ActivateSessionRequest request);
  [[nodiscard]] CloseSessionResponse CloseSession(
      CloseSessionRequest request);

  void DetachSession(const scada::NodeId& authentication_token);
  void PruneExpiredSessions();

  [[nodiscard]] std::optional<ServerSessionLookupResult> FindSession(
      const scada::NodeId& authentication_token) const;

 private:
  struct SessionState {
    scada::NodeId session_id;
    scada::NodeId authentication_token;
    scada::ByteString server_nonce;
    base::TimeDelta revised_timeout;
    base::Time expires_at;
    scada::ServiceContext service_context;
    std::optional<scada::AuthenticationResult> authentication_result;
    bool activated = false;
    bool attached = false;
  };

  [[nodiscard]] base::Time Now() const { return now(); }
  [[nodiscard]] base::TimeDelta ReviseTimeout(base::TimeDelta requested) const;
  [[nodiscard]] scada::NodeId MakeSessionId();
  [[nodiscard]] scada::NodeId MakeAuthenticationToken();
  [[nodiscard]] scada::ByteString MakeServerNonce(scada::UInt32 seed) const;
  [[nodiscard]] SessionState* FindSessionState(
      const scada::NodeId& authentication_token);
  [[nodiscard]] const SessionState* FindSessionState(
      const scada::NodeId& authentication_token) const;
  [[nodiscard]] bool RemoveSessionByUser(const scada::NodeId& user_id);
  [[nodiscard]] bool HasSessionForUser(const scada::NodeId& user_id) const;
  void RemoveSessionByToken(const scada::NodeId& authentication_token);

  std::unordered_map<scada::NodeId, SessionState> sessions_;
  scada::UInt32 next_session_id_ = 1;
  scada::UInt32 next_token_id_ = 1;
};

}  // namespace opcua
