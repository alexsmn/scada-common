#include "events/event_fetcher_builder.h"

#include "base/nested_logger.h"
#include "events/event_ack_queue.h"
#include "events/event_fetcher.h"
#include "events/event_storage.h"
#include "common/coroutine_session_proxy_notifier.h"
#include "scada/coroutine_services.h"

namespace internal {

struct EventFetcherHolderBase {
  EventFetcherHolderBase(AnyExecutor executor,
                         std::shared_ptr<const Logger> logger)
      : executor_{std::move(executor)},
        nested_logger_{
            std::make_shared<NestedLogger>(std::move(logger), "EventFetcher")} {
  }

  AnyExecutor executor_;
  std::shared_ptr<const Logger> nested_logger_;
  EventStorage event_storage_;
};

struct EventFetcherHolder : EventFetcherHolderBase {
  explicit EventFetcherHolder(EventFetcherBuilder&& builder)
      : EventFetcherHolderBase{std::move(builder.executor_),
                               std::move(builder.logger_)},
        services_{builder.services_} {}

  scada::services services_;

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

struct CoroutineEventFetcherHolder : EventFetcherHolderBase {
  explicit CoroutineEventFetcherHolder(CoroutineEventFetcherBuilder&& builder)
      : EventFetcherHolderBase{std::move(builder.executor_),
                               std::move(builder.logger_)},
        monitored_item_service_{builder.monitored_item_service_},
        history_service_{builder.history_service_},
        method_service_{builder.method_service_},
        session_service_{builder.session_service_} {}

  scada::MonitoredItemService& monitored_item_service_;
  scada::CoroutineHistoryService& history_service_;
  scada::CoroutineMethodService& method_service_;
  scada::CoroutineSessionService& session_service_;

  EventAckQueue event_ack_queue_{
      EventAckQueueContext{.logger_ = nested_logger_,
                           .executor_ = executor_,
                           .method_service_ = method_service_}};

  EventFetcher event_fetcher_{EventFetcherContext{
      .executor_ = executor_,
      .monitored_item_service_ = monitored_item_service_,
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

std::shared_ptr<EventFetcher> CoroutineEventFetcherBuilder::Build() {
  auto event_fetcher_holder =
      std::make_shared<internal::CoroutineEventFetcherHolder>(
          std::move(*this));

  return std::shared_ptr<EventFetcher>{event_fetcher_holder,
                                       &event_fetcher_holder->event_fetcher_};
}
