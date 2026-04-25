#include "opcua/client_subscription.h"

#include "opcua/client_session.h"
#include "opcua/client_monitored_item.h"
#include "scada/data_value.h"
#include "scada/date_time.h"

#include <utility>
#include <variant>

namespace opcua {

namespace {

constexpr double kDefaultPublishingIntervalMs = 500.0;
constexpr std::uint32_t kDefaultLifetimeCount = 3000;
constexpr std::uint32_t kDefaultMaxKeepAliveCount = 10000;

}  // namespace

// static
std::shared_ptr<OpcUaClientSubscription> OpcUaClientSubscription::Create(
    OpcUaClientSession& session) {
  return std::shared_ptr<OpcUaClientSubscription>(
      new OpcUaClientSubscription(session));
}

OpcUaClientSubscription::OpcUaClientSubscription(OpcUaClientSession& session)
    : session_{session} {}

OpcUaClientSubscription::~OpcUaClientSubscription() = default;

std::shared_ptr<scada::MonitoredItem>
OpcUaClientSubscription::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params) {
  const std::uint32_t local_id = next_local_id_++;
  return std::make_shared<OpcUaMonitoredItem>(shared_from_this(), local_id,
                                              read_value_id, params);
}

void OpcUaClientSubscription::EnsureCreated() {
  if (impl_ || is_creating_) {
    return;
  }
  is_creating_ = true;
  impl_ = std::make_unique<OpcUaClientProtocolSubscription>(session_.channel());
  auto weak_self = weak_from_this();
  CoSpawn(session_.any_executor(),
          [this, weak_self]() mutable -> Awaitable<void> {
            auto self = weak_self.lock();
            if (!self) {
              co_return;
            }
            OpcUaSubscriptionParameters params{
                .publishing_interval_ms = kDefaultPublishingIntervalMs,
                .lifetime_count = kDefaultLifetimeCount,
                .max_keep_alive_count = kDefaultMaxKeepAliveCount,
                .max_notifications_per_publish = 0,
                .publishing_enabled = true,
                .priority = 0,
            };
            const auto status = co_await impl_->Create(params);
            is_creating_ = false;
            if (status.bad()) {
              impl_.reset();
              co_return;
            }
            StartPublishLoop();
          });
}

void OpcUaClientSubscription::StartPublishLoop() {
  if (publish_loop_running_ || !impl_) {
    return;
  }
  publish_loop_running_ = true;
  auto weak_self = weak_from_this();
  CoSpawn(session_.any_executor(),
          [this, weak_self]() mutable -> Awaitable<void> {
            for (;;) {
              auto self = weak_self.lock();
              if (!self || !impl_ || !session_.is_connected()) {
                break;
              }
              const auto status = co_await impl_->Publish();
              if (status.bad()) {
                break;
              }
            }
            publish_loop_running_ = false;
          });
}

void OpcUaClientSubscription::Subscribe(
    std::uint32_t local_id,
    const scada::ReadValueId& read_value_id,
    const scada::MonitoringParameters& params,
    scada::MonitoredItemHandler handler) {
  EnsureCreated();

  auto weak_self = weak_from_this();
  auto* data_change = std::get_if<scada::DataChangeHandler>(&handler);
  scada::DataChangeHandler dispatch;
  if (data_change) {
    dispatch = *data_change;
  }
  CoSpawn(
      session_.any_executor(),
      [this, local_id, read_value_id, params,
       dispatch = std::move(dispatch),
       weak_self]() mutable -> Awaitable<void> {
        auto self = weak_self.lock();
        if (!self || !impl_) {
          co_return;
        }
        // Wait until Create has completed (best effort — if it hasn't, a
        // single Publish turn will resolve by then; otherwise Subscribe
        // still posts the CreateMonitoredItem request and the server
        // queues it).
        OpcUaMonitoringParameters monitoring{
            .client_handle = 0,  // will be overwritten by impl_
            .sampling_interval_ms =
                params.sampling_interval.has_value()
                    ? params.sampling_interval->InMillisecondsF()
                    : 0.0,
            .filter = std::nullopt,
            .queue_size =
                static_cast<scada::UInt32>(params.queue_size.value_or(1)),
            .discard_oldest = true,
        };
        auto result = co_await impl_->CreateMonitoredItem(
            read_value_id, std::move(monitoring),
            [dispatch](scada::DataValue value) {
              if (dispatch) {
                dispatch(value);
              }
            });
        if (result.ok()) {
          server_ids_by_local_id_[local_id] = result->monitored_item_id;
        }
      });
}

void OpcUaClientSubscription::Unsubscribe(std::uint32_t local_id) {
  if (!impl_) {
    server_ids_by_local_id_.erase(local_id);
    return;
  }
  auto it = server_ids_by_local_id_.find(local_id);
  if (it == server_ids_by_local_id_.end()) {
    return;
  }
  const auto server_id = it->second;
  server_ids_by_local_id_.erase(it);

  auto weak_self = weak_from_this();
  CoSpawn(session_.any_executor(),
          [this, server_id, weak_self]() mutable -> Awaitable<void> {
            auto self = weak_self.lock();
            if (!self || !impl_) {
              co_return;
            }
            co_await impl_->DeleteMonitoredItem(server_id);
          });
}

}  // namespace opcua
