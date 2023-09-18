#include "common/event_fetcher_builder.h"

#include "base/nested_logger.h"
#include "common/event_ack_queue.h"
#include "common/event_fetcher.h"
#include "common/event_fetcher_notifier.h"

namespace internal {

struct EventFetcherHolder : EventFetcherBuilder {
  explicit EventFetcherHolder(EventFetcherBuilder&& builder)
      : EventFetcherBuilder{std::move(builder)} {}

  std::shared_ptr<const Logger> nested_logger_ =
      std::make_shared<NestedLogger>(std::move(logger_), "EventFetcher");

  EventAckQueue event_ack_queue_{
      EventAckQueueContext{.logger_ = nested_logger_,
                           .executor_ = executor_,
                           .method_service_ = method_service_}};

  EventFetcher event_fetcher_{
      EventFetcherContext{.executor_ = executor_,
                          .monitored_item_service_ = monitored_item_service_,
                          .history_service_ = history_service_,
                          .logger_ = nested_logger_,
                          .event_ack_queue_ = event_ack_queue_}};

  EventFetcherNotifier event_fetcher_notifier_{event_fetcher_,
                                               session_service_};
};

}  // namespace internal

std::shared_ptr<EventFetcher> EventFetcherBuilder::Build() {
  auto event_fetcher_holder =
      std::make_shared<internal::EventFetcherHolder>(std::move(*this));

  return std::shared_ptr<EventFetcher>{event_fetcher_holder,
                                       &event_fetcher_holder->event_fetcher_};
}
