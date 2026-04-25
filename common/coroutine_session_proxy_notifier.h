#pragma once

#include "scada/coroutine_services.h"

#include <type_traits>

template <class Proxy>
class CoroutineSessionProxyNotifier {
 public:
  CoroutineSessionProxyNotifier(Proxy& proxy,
                                scada::CoroutineSessionService& session_service)
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
  void OnChannelOpened() {
    if constexpr (std::is_invocable_v<decltype(&Proxy::OnChannelOpened),
                                      Proxy*,
                                      const scada::NodeId&>) {
      proxy_.OnChannelOpened(session_service_.GetUserId());
    } else {
      proxy_.OnChannelOpened();
    }
  }

  Proxy& proxy_;
  scada::CoroutineSessionService& session_service_;

  const boost::signals2::scoped_connection session_state_changed_connection_;
};
