#pragma once

#include "common/event_fetcher.h"
#include "core/session_service.h"

class EventFetcherNotifier {
 public:
  EventFetcherNotifier(EventFetcher& proxy,
                       scada::SessionService& session_service)
      : event_fetcher_{proxy},
        session_state_changed_connection_{
            session_service.SubscribeSessionStateChanged(
                [&proxy, &session_service](bool connected,
                                           const scada::Status& status) {
                  if (connected)
                    proxy.OnChannelOpened(session_service.GetUserId());
                  else
                    proxy.OnChannelClosed();
                })} {
    if (session_service.IsConnected())
      event_fetcher_.OnChannelOpened(session_service.GetUserId());
  }

 private:
  EventFetcher& event_fetcher_;

  const boost::signals2::scoped_connection session_state_changed_connection_;
};
