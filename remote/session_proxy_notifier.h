#pragma once

#include "core/session_service.h"
#include "core/session_state_observer.h"

template <class Proxy>
class SessionProxyNotifier : private scada::SessionStateObserver {
 public:
  SessionProxyNotifier(Proxy& proxy, scada::SessionService& session_service);
  ~SessionProxyNotifier();

 private:
  // scada::SessionStateObserver
  virtual void OnSessionCreated() override;
  virtual void OnSessionDeleted(const scada::Status& status) override;

  Proxy& proxy_;
  scada::SessionService& session_service_;
};

template <class Proxy>
inline SessionProxyNotifier<Proxy>::SessionProxyNotifier(
    Proxy& proxy,
    scada::SessionService& session_service)
    : proxy_{proxy}, session_service_{session_service} {
  session_service_.AddObserver(*this);
}

template <class Proxy>
inline SessionProxyNotifier<Proxy>::~SessionProxyNotifier() {
  session_service_.RemoveObserver(*this);
}

template <class Proxy>
inline void SessionProxyNotifier<Proxy>::OnSessionCreated() {
  proxy_.OnChannelOpened();
}

template <class Proxy>
inline void SessionProxyNotifier<Proxy>::OnSessionDeleted(
    const scada::Status& status) {
  proxy_.OnChannelClosed();
}
