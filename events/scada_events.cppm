// scada.events — named C++20 module facade over the common/events headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md). `export import scada.common;` mirrors
// scada_common_events's PUBLIC link.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "events/event_ack_queue.h"
#include "events/event_fetcher.h"
#include "events/event_fetcher_builder.h"
#include "events/event_notifier.h"
#include "events/event_observer.h"
#include "events/event_set.h"
#include "events/event_storage.h"
#include "events/node_event_provider.h"
#include "events/view_events_subscription.h"

export module scada.events;

export import scada.common;

export {
  // event_ack_queue.h
  using ::EventAckQueue;
  using ::EventAckQueueContext;

  // event_fetcher.h / event_fetcher_builder.h
  using ::CoroutineEventFetcherBuilder;
  using ::EventFetcher;
  using ::EventFetcherBuilder;
  using ::EventFetcherContext;

  // event_notifier.h / event_observer.h
  using ::EventNotifier;
  using ::EventObserver;

  // event_set.h / event_storage.h / node_event_provider.h
  using ::EventComparer;
  using ::EventSet;
  using ::EventStorage;
  using ::NodeEventProvider;

  // view_events_subscription.h
  using ::CreateModelAndSemanticChangeEventsMonitoredItem;
  using ::CreateModelChangeEventsMonitoredItem;
  using ::IViewEventsSubscription;
  using ::MakeViewEventsProvider;
  using ::ViewEventsProvider;
  using ::ViewEventsSubscription;
}  // export
