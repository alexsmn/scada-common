#include "events/event_fetcher_builder.h"

#include "base/nested_logger.h"
#include "common/coroutine_session_proxy_notifier.h"
#include "events/event_ack_queue.h"
#include "events/event_fetcher.h"
#include "events/event_storage.h"
#include "scada/coroutine_services.h"
#include "scada/monitored_item_service.h"

namespace internal {

struct EventFetcherHolderBase {
  EventFetcherHolderBase(AnyExecutor executor,
                         std::shared_ptr<const Logger> logger)
      : executor_{std::move(executor)},
        nested_logger_{std::make_shared<NestedLogger>(std::move(logger),
                                                      "EventFetcher")} {}

  AnyExecutor executor_;
  std::shared_ptr<const Logger> nested_logger_;
  EventStorage event_storage_;
};

bool HasDataServices(const DataServices& services) {
  return services.session_service_ || services.view_service_ ||
         services.node_management_service_ || services.history_service_ ||
         services.attribute_service_ || services.method_service_ ||
         services.monitored_item_service_ ||
         services.coroutine_session_service_ ||
         services.coroutine_view_service_ ||
         services.coroutine_node_management_service_ ||
         services.coroutine_history_service_ ||
         services.coroutine_attribute_service_ ||
         services.coroutine_method_service_;
}

void NormalizeLegacyServices(EventFetcherBuilder& builder) {
  if (HasDataServices(builder.data_services_))
    return;

  builder.data_services_ = DataServices::FromUnownedServices(builder.services_);
}

struct EventFetcherServices {
  EventFetcherServices(AnyExecutor executor, EventFetcherBuilder& builder)
      : data_services_{std::move(builder.data_services_)} {
    monitored_item_service_ = data_services_.monitored_item_service_.get();

    history_service_ = data_services_.coroutine_history_service_.get();
    if (!history_service_ && data_services_.history_service_) {
      if (auto* service = dynamic_cast<scada::CoroutineHistoryService*>(
              data_services_.history_service_.get())) {
        history_service_ = service;
      } else {
        history_service_adapter_ =
            std::make_unique<scada::CallbackToCoroutineHistoryServiceAdapter>(
                executor, *data_services_.history_service_);
        history_service_ = history_service_adapter_.get();
      }
    }

    method_service_ = data_services_.coroutine_method_service_.get();
    if (!method_service_ && data_services_.method_service_) {
      if (auto* service = dynamic_cast<scada::CoroutineMethodService*>(
              data_services_.method_service_.get())) {
        method_service_ = service;
      } else {
        method_service_adapter_ =
            std::make_unique<scada::CallbackToCoroutineMethodServiceAdapter>(
                executor, *data_services_.method_service_);
        method_service_ = method_service_adapter_.get();
      }
    }

    session_service_ = data_services_.coroutine_session_service_.get();
    if (!session_service_ && data_services_.session_service_) {
      if (auto* service = dynamic_cast<scada::CoroutineSessionService*>(
              data_services_.session_service_.get())) {
        session_service_ = service;
      } else {
        session_service_adapter_ =
            std::make_unique<scada::PromiseToCoroutineSessionServiceAdapter>(
                executor, *data_services_.session_service_);
        session_service_ = session_service_adapter_.get();
      }
    }

    assert(monitored_item_service_);
    assert(history_service_);
    assert(method_service_);
    assert(session_service_);
  }

  DataServices data_services_;
  std::unique_ptr<scada::CallbackToCoroutineHistoryServiceAdapter>
      history_service_adapter_;
  std::unique_ptr<scada::CallbackToCoroutineMethodServiceAdapter>
      method_service_adapter_;
  std::unique_ptr<scada::PromiseToCoroutineSessionServiceAdapter>
      session_service_adapter_;
  scada::MonitoredItemService* monitored_item_service_ = nullptr;
  scada::CoroutineHistoryService* history_service_ = nullptr;
  scada::CoroutineMethodService* method_service_ = nullptr;
  scada::CoroutineSessionService* session_service_ = nullptr;
};

struct EventFetcherHolder : EventFetcherHolderBase {
  explicit EventFetcherHolder(EventFetcherBuilder&& builder)
      : EventFetcherHolderBase{std::move(builder.executor_),
                               std::move(builder.logger_)},
        services_{executor_, builder} {}

  EventFetcherServices services_;

  EventAckQueue event_ack_queue_{
      EventAckQueueContext{.logger_ = nested_logger_,
                           .executor_ = executor_,
                           .method_service_ = *services_.method_service_}};

  EventFetcher event_fetcher_{EventFetcherContext{
      .executor_ = executor_,
      .monitored_item_service_ = *services_.monitored_item_service_,
      .history_service_ = *services_.history_service_,
      .logger_ = nested_logger_,
      .event_storage_ = event_storage_,
      .event_ack_queue_ = event_ack_queue_}};

  CoroutineSessionProxyNotifier<EventFetcher> event_fetcher_notifier_{
      event_fetcher_, *services_.session_service_};
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

  EventFetcher event_fetcher_{
      EventFetcherContext{.executor_ = executor_,
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
  internal::NormalizeLegacyServices(*this);
  auto event_fetcher_holder =
      std::make_shared<internal::EventFetcherHolder>(std::move(*this));

  return std::shared_ptr<EventFetcher>{event_fetcher_holder,
                                       &event_fetcher_holder->event_fetcher_};
}

std::shared_ptr<EventFetcher> CoroutineEventFetcherBuilder::Build() {
  auto event_fetcher_holder =
      std::make_shared<internal::CoroutineEventFetcherHolder>(std::move(*this));

  return std::shared_ptr<EventFetcher>{event_fetcher_holder,
                                       &event_fetcher_holder->event_fetcher_};
}
