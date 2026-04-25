#include "events/event_fetcher_builder.h"

#include "base/nested_logger.h"
#include "events/event_ack_queue.h"
#include "events/event_fetcher.h"
#include "events/event_storage.h"
#include "common/coroutine_session_proxy_notifier.h"
#include "scada/coroutine_services.h"

namespace internal {

struct EventFetcherHolder : EventFetcherBuilder {
  explicit EventFetcherHolder(EventFetcherBuilder&& builder)
      : EventFetcherBuilder{std::move(builder)} {}

  std::shared_ptr<const Logger> nested_logger_ =
      std::make_shared<NestedLogger>(std::move(logger_), "EventFetcher");

  EventStorage event_storage_;

  scada::CallbackToCoroutineMethodServiceAdapter method_service_{
      executor_, *services_.method_service};

  EventAckQueue event_ack_queue_{
      EventAckQueueContext{.logger_ = nested_logger_,
                           .executor_ = executor_,
                           .method_service_ = method_service_}};

  scada::CallbackToCoroutineHistoryServiceAdapter history_service_{
      executor_, *services_.history_service};

  scada::PromiseToCoroutineSessionServiceAdapter session_service_{
      executor_, *services_.session_service};

  EventFetcher event_fetcher_{EventFetcherContext{
      .executor_ = executor_,
      .monitored_item_service_ = *services_.monitored_item_service,
      .history_service_ = history_service_,
      .logger_ = nested_logger_,
      .event_storage_ = event_storage_,
      .event_ack_queue_ = event_ack_queue_}};

  CoroutineSessionProxyNotifier<EventFetcher> event_fetcher_notifier_{
      event_fetcher_, session_service_};
};

}  // namespace internal

std::shared_ptr<EventFetcher> EventFetcherBuilder::Build() {
  auto event_fetcher_holder =
      std::make_shared<internal::EventFetcherHolder>(std::move(*this));

  return std::shared_ptr<EventFetcher>{event_fetcher_holder,
                                       &event_fetcher_holder->event_fetcher_};
}
