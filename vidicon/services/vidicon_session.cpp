#include "vidicon/services/vidicon_session.h"

#include "base/win/scoped_bstr.h"

#include "scada/date_time.h"
#include "scada/history_types.h"
#include "scada/item_factory_subscription.h"
#include "scada/monitored_item.h"
#include "scada/standard_node_ids.h"
#include "vidicon/services/vidicon_monitored_data_point.h"
#include "vidicon/services/vidicon_monitored_events.h"
#include <format>

#include <string_view>
#include <wrl/client.h>

namespace {

Microsoft::WRL::ComPtr<IClient> CreateTeleClient() {
  Microsoft::WRL::ComPtr<IClient> client;
  ::CoCreateInstance(__uuidof(Client), nullptr, CLSCTX_ALL,
                     IID_PPV_ARGS(&client));
  return client;
}

}  // namespace

VidiconSession::VidiconSession()
    : sync_attribute_service_{{address_space_}},
      sync_view_service_{{address_space_}} {}

VidiconSession::~VidiconSession() {}

Awaitable<void> VidiconSession::Connect(scada::SessionConnectParams params) {
  (void)co_await ConnectStatus(std::move(params));
}

Awaitable<scada::Status> VidiconSession::ConnectStatus(
    scada::SessionConnectParams params) {
  teleclient_ = CreateTeleClient();
  if (!teleclient_) {
    co_return scada::StatusCode::Bad;
  }

  co_return scada::StatusCode::Good;
}

Awaitable<void> VidiconSession::Reconnect() {
  co_return;
}

Awaitable<void> VidiconSession::Disconnect() {
  teleclient_.Reset();

  co_return;
}

bool VidiconSession::IsConnected(scada::base::TimeDelta* ping_delay) const {
  return true;
}

bool VidiconSession::HasPrivilege(scada::Privilege privilege) const {
  return true;
}

scada::NodeId VidiconSession::GetUserId() const {
  return {};
}

std::string VidiconSession::GetHostName() const {
  return "Vidicon";
}

boost::signals2::scoped_connection VidiconSession::SubscribeSessionStateChanged(
    const SessionStateChangedCallback& callback) {
  return boost::signals2::scoped_connection{};
}

Awaitable<scada::HistoryReadRawResult> VidiconSession::HistoryReadRaw(
    scada::HistoryReadRawDetails details) {
  co_return scada::HistoryReadRawResult{.status = scada::StatusCode::Bad};
}

Awaitable<scada::HistoryReadEventsResult> VidiconSession::HistoryReadEvents(
    scada::NodeId node_id,
    scada::base::Time from,
    scada::base::Time to,
    scada::EventFilter filter) {
  co_return scada::HistoryReadEventsResult{.status = scada::StatusCode::Bad};
}

scada::StatusOr<std::unique_ptr<scada::MonitoredItemSubscription>>
VidiconSession::CreateSubscription(
    scada::ServiceContext /*context*/,
    scada::MonitoredItemSubscriptionOptions options) {
  return scada::MakeItemFactorySubscription(
      [this](const scada::ReadValueId& read_value_id,
             const scada::MonitoringParameters& params)
          -> std::shared_ptr<scada::MonitoredItem> {
        if (read_value_id.attribute_id == scada::AttributeId::Value) {
          if (read_value_id.node_id.type() != scada::NodeIdType::Numeric)
            return nullptr;
          auto address =
              std::format(L"CF:{}", read_value_id.node_id.numeric_id());
          Microsoft::WRL::ComPtr<IDataPoint> point;
          teleclient_->RequestPoint(scada::base::win::ScopedBstr(address),
                                    point.GetAddressOf());
          if (!point)
            return nullptr;
          return std::make_shared<VidiconMonitoredDataPoint>(std::move(point));
        }

        if (read_value_id.attribute_id == scada::AttributeId::EventNotifier) {
          // The read id comes from a subscription request (external input);
          // only the Server node exposes events.
          if (read_value_id.node_id != scada::id::Server)
            return nullptr;
          return std::make_shared<VidiconMonitoredEvents>();
        }

        return nullptr;
      },
      options);
}

Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> VidiconSession::Read(
    scada::ServiceContext context,
    std::vector<scada::ReadValueId> inputs) {
  co_return co_await attribute_service_.Read(std::move(context),
                                             std::move(inputs));
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
VidiconSession::Write(scada::ServiceContext context,
                      std::vector<scada::WriteValue> inputs) {
  co_return scada::Status{scada::StatusCode::Bad};
}

Awaitable<scada::Status> VidiconSession::Call(
    scada::NodeId node_id,
    scada::NodeId method_id,
    std::vector<scada::Variant> arguments,
    scada::ServiceContext context) {
  co_return scada::Status{scada::StatusCode::Bad};
}

Awaitable<scada::StatusOr<std::vector<scada::AddNodesResult>>>
VidiconSession::AddNodes(scada::ServiceContext /*context*/,
                         std::vector<scada::AddNodesItem> inputs) {
  co_return scada::Status{scada::StatusCode::Bad};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
VidiconSession::DeleteNodes(scada::ServiceContext /*context*/,
                            std::vector<scada::DeleteNodesItem> inputs) {
  co_return scada::Status{scada::StatusCode::Bad};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
VidiconSession::AddReferences(scada::ServiceContext /*context*/,
                              std::vector<scada::AddReferencesItem> inputs) {
  co_return scada::Status{scada::StatusCode::Bad};
}

Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>>
VidiconSession::DeleteReferences(
    scada::ServiceContext /*context*/,
    std::vector<scada::DeleteReferencesItem> inputs) {
  co_return scada::Status{scada::StatusCode::Bad};
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
VidiconSession::Browse(scada::ServiceContext context,
                       std::vector<scada::BrowseDescription> inputs) {
  co_return co_await view_service_.Browse(std::move(context),
                                          std::move(inputs));
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
VidiconSession::TranslateBrowsePaths(std::vector<scada::BrowsePath> inputs) {
  co_return co_await view_service_.TranslateBrowsePaths(std::move(inputs));
}

scada::SessionDebugger* VidiconSession::GetSessionDebugger() {
  return nullptr;
}
