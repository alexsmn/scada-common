#pragma once

// Server-side service adapters: present SCADA `core` services as an opcuapp
// callback bundle. Each method converts its arguments core<-opcua, calls the
// inner core service, and converts the result opcua<-core. The Awaitable type
// is the same boost::asio::awaitable on both sides, so a core coroutine can be
// co_awaited directly inside an opcua one.

#include "base/lifetime.h"
#include "opcua_bridge/service_conversion.h"

#include "scada/attribute_service.h"
#include "scada/authentication.h"
#include "scada/history_service.h"
#include "scada/method_service.h"
#include "scada/monitored_item_service.h"
#include "scada/node_management_service.h"
#include "scada/view_service.h"

#include "opcua/scada/attribute_service.h"
#include "opcua/scada/authentication.h"
#include "opcua/scada/history_service.h"
#include "opcua/scada/method_service.h"
#include "opcua/scada/node_management_service.h"
#include "opcua/scada/view_service.h"
#include "opcua/service_callbacks.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace opcua_bridge {

class AttributeServiceAdapter {
 public:
  explicit AttributeServiceAdapter(
      scada::AttributeService& inner SCADA_LIFETIME_BOUND)
      : inner_{inner} {}

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::DataValue>>> Read(
      opcua::ServiceContext context,
      std::shared_ptr<const std::vector<opcua::ReadValueId>> inputs);

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>> Write(
      opcua::ServiceContext context,
      std::shared_ptr<const std::vector<opcua::WriteValue>> inputs);

 private:
  scada::AttributeService& inner_;
};

class ViewServiceAdapter {
 public:
  explicit ViewServiceAdapter(scada::ViewService& inner SCADA_LIFETIME_BOUND)
      : inner_{inner} {}

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::BrowseResult>>> Browse(
      opcua::ServiceContext context,
      std::vector<opcua::BrowseDescription> inputs);

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::BrowsePathResult>>>
  TranslateBrowsePaths(std::vector<opcua::BrowsePath> inputs);

 private:
  scada::ViewService& inner_;
};

class MethodServiceAdapter {
 public:
  explicit MethodServiceAdapter(
      scada::MethodService& inner SCADA_LIFETIME_BOUND)
      : inner_{inner} {}

  opcua::Awaitable<opcua::Status> Call(opcua::NodeId node_id,
                                       opcua::NodeId method_id,
                                       std::vector<opcua::Variant> arguments,
                                       opcua::NodeId user_id);

 private:
  scada::MethodService& inner_;
};

class NodeManagementServiceAdapter {
 public:
  explicit NodeManagementServiceAdapter(
      scada::NodeManagementService& inner SCADA_LIFETIME_BOUND)
      : inner_{inner} {}

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::AddNodesResult>>>
  AddNodes(std::vector<opcua::AddNodesItem> inputs);

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>> DeleteNodes(
      std::vector<opcua::DeleteNodesItem> inputs);

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
  AddReferences(std::vector<opcua::AddReferencesItem> inputs);

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
  DeleteReferences(std::vector<opcua::DeleteReferencesItem> inputs);

 private:
  scada::NodeManagementService& inner_;
};

class HistoryServiceAdapter {
 public:
  explicit HistoryServiceAdapter(
      scada::HistoryService& inner SCADA_LIFETIME_BOUND)
      : inner_{inner} {}

  opcua::Awaitable<opcua::HistoryReadRawResult> HistoryReadRaw(
      opcua::HistoryReadRawDetails details);

  opcua::Awaitable<opcua::HistoryReadEventsResult> HistoryReadEvents(
      opcua::NodeId node_id,
      opcua::base::Time from,
      opcua::base::Time to,
      opcua::EventFilter filter);

 private:
  scada::HistoryService& inner_;
};

// Wraps an inner core MonitoredItemSubscription as the opcua interface. Event
// notifications cross the boundary as the standard wire `EventFieldList`; this
// adapter projects each core event onto the monitored item's EventFilter select
// clauses, so it stores the parsed field paths per client_handle as items are
// added.
class MonitoredItemSubscriptionAdapter
    : public opcua::scada::MonitoredItemSubscription {
 public:
  explicit MonitoredItemSubscriptionAdapter(
      std::unique_ptr<scada::MonitoredItemSubscription> inner)
      : inner_{std::move(inner)} {}

  opcua::Awaitable<std::vector<opcua::MonitoredItemCreateResult>> AddItems(
      std::vector<opcua::MonitoredItemCreateRequest> requests) override;

  opcua::Awaitable<std::vector<opcua::Status>> RemoveItems(
      std::span<const opcua::scada::MonitoredItemId> item_ids) override;

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::scada::ItemNotification>>>
  ReadNext(std::size_t max_count) override;

  void Close(opcua::Status status) override;

 private:
  // Converts a single core notification to its standard wire form, projecting
  // core events onto the per-item EventFilter select clauses stored in
  // `field_paths_by_handle_`.
  opcua::scada::ItemNotification ToItemNotification(
      const scada::MonitoredItemNotification& notification) const;

  std::unique_ptr<scada::MonitoredItemSubscription> inner_;
  // Event-field select-clause paths parsed from each item's wire filter, keyed
  // by client_handle. Populated in AddItems; consumed in ReadNext to project
  // events into EventFieldList.
  std::unordered_map<std::uint32_t, std::vector<std::vector<std::string>>>
      field_paths_by_handle_;
};

class MonitoredItemServiceAdapter {
 public:
  explicit MonitoredItemServiceAdapter(
      scada::MonitoredItemService& inner SCADA_LIFETIME_BOUND)
      : inner_{inner} {}

  opcua::StatusOr<std::unique_ptr<opcua::scada::MonitoredItemSubscription>>
  CreateSubscription(opcua::ServiceContext context,
                     opcua::scada::MonitoredItemSubscriptionOptions options);

 private:
  scada::MonitoredItemService& inner_;
};

// Presents a core authenticator as the opcua interface the server session
// manager consumes.
class AuthenticatorAdapter : public opcua::CoroutineAuthenticator {
 public:
  explicit AuthenticatorAdapter(
      std::shared_ptr<scada::CoroutineAuthenticator> inner)
      : inner_{std::move(inner)} {}

  opcua::Awaitable<opcua::StatusOr<opcua::AuthenticationResult>> Authenticate(
      opcua::LocalizedText user_name,
      opcua::LocalizedText password) override;

 private:
  std::shared_ptr<scada::CoroutineAuthenticator> inner_;
};

// Owns the six adapters wrapping a set of core services for opcuapp's server
// runtime. The returned callback bundle captures this object and must not
// outlive it.
struct ServerServiceAdapters {
  ServerServiceAdapters(
      scada::AttributeService& attribute SCADA_LIFETIME_BOUND,
      scada::ViewService& view SCADA_LIFETIME_BOUND,
      scada::MethodService& method SCADA_LIFETIME_BOUND,
      scada::NodeManagementService& node_management SCADA_LIFETIME_BOUND,
      scada::HistoryService& history SCADA_LIFETIME_BOUND,
      scada::MonitoredItemService& monitored_item SCADA_LIFETIME_BOUND)
      : attribute{attribute},
        view{view},
        method{method},
        node_management{node_management},
        history{history},
        monitored_item{monitored_item},
        callbacks_{MakeCallbacks()} {}

  const opcua::ServiceCallbacks& callbacks() const SCADA_LIFETIME_BOUND {
    return callbacks_;
  }

  AttributeServiceAdapter attribute;
  ViewServiceAdapter view;
  MethodServiceAdapter method;
  NodeManagementServiceAdapter node_management;
  HistoryServiceAdapter history;
  MonitoredItemServiceAdapter monitored_item;

 private:
  opcua::ServiceCallbacks MakeCallbacks() SCADA_LIFETIME_BOUND;

  opcua::ServiceCallbacks callbacks_;
};

}  // namespace opcua_bridge
