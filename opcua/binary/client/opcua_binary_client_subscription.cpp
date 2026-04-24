#include "opcua/binary/client/opcua_binary_client_subscription.h"

#include <utility>
#include <variant>

namespace opcua {
namespace {

template <typename Response>
scada::StatusOr<Response> NarrowResponse(
    scada::StatusOr<OpcUaResponseBody> result) {
  if (!result.ok()) {
    return scada::StatusOr<Response>{result.status()};
  }
  if (auto* fault = std::get_if<OpcUaServiceFault>(&result.value())) {
    return scada::StatusOr<Response>{fault->status};
  }
  if (auto* typed = std::get_if<Response>(&result.value())) {
    return scada::StatusOr<Response>{std::move(*typed)};
  }
  return scada::StatusOr<Response>{scada::Status{scada::StatusCode::Bad}};
}

}  // namespace

OpcUaBinaryClientSubscription::OpcUaBinaryClientSubscription(
    OpcUaBinaryClientChannel& channel)
    : channel_{channel} {}

Awaitable<scada::Status> OpcUaBinaryClientSubscription::Create(
    OpcUaSubscriptionParameters parameters) {
  const auto handle = channel_.NextRequestHandle();
  auto result = co_await channel_.Call(
      handle, OpcUaRequestBody{OpcUaCreateSubscriptionRequest{
                  .parameters = std::move(parameters)}});
  auto narrowed = NarrowResponse<OpcUaCreateSubscriptionResponse>(
      std::move(result));
  if (!narrowed.ok()) {
    co_return narrowed.status();
  }
  if (narrowed->status.bad()) {
    co_return narrowed->status;
  }
  subscription_id_ = narrowed->subscription_id;
  is_created_ = true;
  co_return scada::Status{scada::StatusCode::Good};
}

Awaitable<scada::StatusOr<OpcUaBinaryClientSubscription::CreateMonitoredItemResult>>
OpcUaBinaryClientSubscription::CreateMonitoredItem(
    scada::ReadValueId read_value_id,
    OpcUaMonitoringParameters params,
    DataChangeHandler handler) {
  if (!is_created_) {
    co_return scada::StatusOr<CreateMonitoredItemResult>{
        scada::Status{scada::StatusCode::Bad}};
  }
  const scada::UInt32 client_handle = next_client_handle_++;
  params.client_handle = client_handle;

  const auto request_handle = channel_.NextRequestHandle();
  auto result = co_await channel_.Call(
      request_handle,
      OpcUaRequestBody{OpcUaCreateMonitoredItemsRequest{
          .subscription_id = subscription_id_,
          .timestamps_to_return = OpcUaTimestampsToReturn::Both,
          .items_to_create = {OpcUaMonitoredItemCreateRequest{
              .item_to_monitor = std::move(read_value_id),
              .monitoring_mode = OpcUaMonitoringMode::Reporting,
              .requested_parameters = std::move(params),
          }},
      }});
  auto narrowed = NarrowResponse<OpcUaCreateMonitoredItemsResponse>(
      std::move(result));
  if (!narrowed.ok()) {
    co_return scada::StatusOr<CreateMonitoredItemResult>{narrowed.status()};
  }
  if (narrowed->status.bad() || narrowed->results.empty()) {
    co_return scada::StatusOr<CreateMonitoredItemResult>{
        narrowed->results.empty()
            ? scada::Status{scada::StatusCode::Bad}
            : narrowed->status};
  }
  const auto& item_result = narrowed->results.front();
  if (item_result.status.bad()) {
    co_return scada::StatusOr<CreateMonitoredItemResult>{item_result.status};
  }
  handlers_.emplace(client_handle, std::move(handler));
  client_handle_by_item_id_.emplace(item_result.monitored_item_id,
                                     client_handle);
  co_return scada::StatusOr<CreateMonitoredItemResult>{
      CreateMonitoredItemResult{
          .monitored_item_id = item_result.monitored_item_id,
          .client_handle = client_handle,
      }};
}

Awaitable<scada::Status> OpcUaBinaryClientSubscription::DeleteMonitoredItem(
    OpcUaMonitoredItemId monitored_item_id) {
  if (!is_created_) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  const auto handle = channel_.NextRequestHandle();
  auto result = co_await channel_.Call(
      handle, OpcUaRequestBody{OpcUaDeleteMonitoredItemsRequest{
                  .subscription_id = subscription_id_,
                  .monitored_item_ids = {monitored_item_id},
              }});
  auto narrowed = NarrowResponse<OpcUaDeleteMonitoredItemsResponse>(
      std::move(result));
  if (!narrowed.ok()) {
    co_return narrowed.status();
  }
  // Drop the handler regardless of server response — on a bad response the
  // server may or may not have removed it, but our client state is
  // unambiguous.
  if (auto it = client_handle_by_item_id_.find(monitored_item_id);
      it != client_handle_by_item_id_.end()) {
    handlers_.erase(it->second);
    client_handle_by_item_id_.erase(it);
  }
  co_return narrowed->status;
}

Awaitable<scada::Status> OpcUaBinaryClientSubscription::Publish() {
  if (!is_created_) {
    co_return scada::Status{scada::StatusCode::Bad};
  }
  auto acks = std::move(pending_acks_);
  pending_acks_.clear();

  const auto handle = channel_.NextRequestHandle();
  auto result = co_await channel_.Call(
      handle, OpcUaRequestBody{OpcUaPublishRequest{
                  .subscription_acknowledgements = std::move(acks)}});
  auto narrowed = NarrowResponse<OpcUaPublishResponse>(std::move(result));
  if (!narrowed.ok()) {
    co_return narrowed.status();
  }
  if (narrowed->status.bad()) {
    co_return narrowed->status;
  }

  // Queue ack for the sequence number we just received so it goes out on
  // the next PublishRequest.
  pending_acks_.push_back(OpcUaSubscriptionAcknowledgement{
      .subscription_id = narrowed->subscription_id,
      .sequence_number = narrowed->notification_message.sequence_number,
  });

  // Dispatch data-change notifications to handlers by client_handle.
  for (const auto& data : narrowed->notification_message.notification_data) {
    if (const auto* change = std::get_if<OpcUaDataChangeNotification>(&data)) {
      for (const auto& item : change->monitored_items) {
        if (auto it = handlers_.find(item.client_handle);
            it != handlers_.end() && it->second) {
          it->second(item.value);
        }
      }
    }
    // Event + StatusChange notifications are intentionally not wired up in
    // this revision; the existing client didn't consume them either.
  }
  co_return scada::Status{scada::StatusCode::Good};
}

Awaitable<scada::Status> OpcUaBinaryClientSubscription::Delete() {
  if (!is_created_) {
    co_return scada::Status{scada::StatusCode::Good};
  }
  const auto handle = channel_.NextRequestHandle();
  auto result = co_await channel_.Call(
      handle, OpcUaRequestBody{OpcUaDeleteSubscriptionsRequest{
                  .subscription_ids = {subscription_id_}}});
  is_created_ = false;
  handlers_.clear();
  client_handle_by_item_id_.clear();
  pending_acks_.clear();
  auto narrowed = NarrowResponse<OpcUaDeleteSubscriptionsResponse>(
      std::move(result));
  if (!narrowed.ok()) {
    co_return narrowed.status();
  }
  co_return narrowed->status;
}

}  // namespace opcua
