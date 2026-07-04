#include "events/event_fetcher_builder.h"

#include "base/nested_logger.h"
#include "common/coroutine_service_resolver.h"
#include "common/data_services_util.h"
#include "common/session_proxy_notifier.h"
#include "events/event_ack_queue.h"
#include "events/event_fetcher.h"
#include "events/event_storage.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/session_service.h"

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

void NormalizeLegacyServices(EventFetcherBuilder& builder) {
  if (data_services::HasServices(builder.data_services_))
    return;

  builder.data_services_ =
      data_services::FromUnownedServices(builder.services_);
}

bool HasRequiredServices(const DataServices& data_services) {
  return data_services.monitored_item_service_ &&
         data_services.history_service_ && data_services.method_service_ &&
         data_services.session_service_;
}

DataServices MakeDataServices(CoroutineEventFetcherBuilder& builder) {
  return DataServices{
      .session_service_ = data_services::Unowned(builder.session_service_),
      .history_service_ = data_services::Unowned(builder.history_service_),
      .method_service_ = data_services::Unowned(builder.method_service_),
      .monitored_item_service_ =
          data_services::Unowned(builder.monitored_item_service_)};
}

struct EventFetcherServices {
  EventFetcherServices(AnyExecutor executor, EventFetcherBuilder& builder)
      : data_services_{std::move(builder.data_services_)} {
    monitored_item_service_ = &scada::service_resolver::RequireSharedService(
        data_services_.monitored_item_service_);

    history_service_ = &scada::service_resolver::RequireSharedService(
        data_services_.history_service_);
    method_service_ = &scada::service_resolver::RequireSharedService(
        data_services_.method_service_);
    session_service_ = &scada::service_resolver::RequireSharedService(
        data_services_.session_service_);
  }

  DataServices data_services_;
  scada::MonitoredItemService* monitored_item_service_ = nullptr;
  scada::HistoryService* history_service_ = nullptr;
  scada::MethodService* method_service_ = nullptr;
  scada::SessionService* session_service_ = nullptr;
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

  SessionProxyNotifier<EventFetcher> event_fetcher_notifier_{
      event_fetcher_, *services_.session_service_};
};

}  // namespace internal

std::shared_ptr<EventFetcher> EventFetcherBuilder::Build() {
  internal::NormalizeLegacyServices(*this);
  if (!internal::HasRequiredServices(data_services_))
    return nullptr;

  auto event_fetcher_holder =
      std::make_shared<internal::EventFetcherHolder>(std::move(*this));

  return std::shared_ptr<EventFetcher>{event_fetcher_holder,
                                       &event_fetcher_holder->event_fetcher_};
}

std::shared_ptr<EventFetcher> CoroutineEventFetcherBuilder::Build() {
  return EventFetcherBuilder{
      .executor_ = std::move(executor_),
      .logger_ = std::move(logger_),
      .data_services_ = internal::MakeDataServices(*this)}
      .Build();
}
