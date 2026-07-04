#pragma once

#include "scada/service_context.h"
#include "scada/session_service.h"

#include <cstdint>
#include <type_traits>

template <class Proxy>
class SessionProxyNotifier {
 public:
  SessionProxyNotifier(Proxy& proxy,
                                scada::SessionService& session_service)
      : proxy_{proxy},
        session_service_{session_service},
        session_state_changed_connection_{
            session_service.SubscribeSessionStateChanged(
                [this](bool connected, const scada::Status& status) {
                  if (connected)
                    OnChannelOpened();
                  else
                    proxy_.OnChannelClosed();
                })} {
    if (session_service_.IsConnected())
      OnChannelOpened();
  }

 private:
  // Builds the caller's ServiceContext from the live session: its user id plus a
  // rights bitmask reconstructed from the privileges the session exposes. The
  // bit layout matches scada::Privilege (bit N = 1u << Privilege), so downstream
  // permission checks (scada::IsPermitted) see the acknowledging user's real
  // rights rather than a system identity.
  scada::ServiceContext MakeSessionContext() const {
    std::uint32_t user_rights = 0;
    if (session_service_.HasPrivilege(scada::Privilege::Configure))
      user_rights |= 1u << static_cast<int>(scada::Privilege::Configure);
    if (session_service_.HasPrivilege(scada::Privilege::Control))
      user_rights |= 1u << static_cast<int>(scada::Privilege::Control);
    return scada::ServiceContext{}
        .with_user_id(session_service_.GetUserId())
        .with_user_rights(user_rights);
  }

  void OnChannelOpened() {
    if constexpr (std::is_invocable_v<decltype(&Proxy::OnChannelOpened),
                                      Proxy*,
                                      const scada::ServiceContext&>) {
      proxy_.OnChannelOpened(MakeSessionContext());
    } else if constexpr (std::is_invocable_v<decltype(&Proxy::OnChannelOpened),
                                             Proxy*,
                                             const scada::NodeId&>) {
      proxy_.OnChannelOpened(session_service_.GetUserId());
    } else {
      proxy_.OnChannelOpened();
    }
  }

  Proxy& proxy_;
  scada::SessionService& session_service_;

  const boost::signals2::scoped_connection session_state_changed_connection_;
};
