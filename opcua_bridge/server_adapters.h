#pragma once

// Server-side service adapters: present a SCADA `core` service implementation
// (scada::*Service) as the corresponding opcuapp interface (opcua::scada::
// *Service) that opcuapp's server runtime consumes. Each method converts its
// arguments core<-opcua, calls the inner core service, and converts the result
// opcua<-core. The Awaitable type is the same boost::asio::awaitable on both
// sides, so a core coroutine can be co_awaited directly inside an opcua one.

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
#include "opcua/scada/monitored_item_service.h"
#include "opcua/scada/node_management_service.h"
#include "opcua/scada/view_service.h"

#include <memory>
#include <span>
#include <vector>

namespace opcua_bridge {

class AttributeServiceAdapter : public opcua::scada::AttributeService {
 public:
  explicit AttributeServiceAdapter(scada::AttributeService& inner)
      : inner_{inner} {}

  opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::DataValue>>>
  Read(opcua::scada::ServiceContext context,
       std::shared_ptr<const std::vector<opcua::scada::ReadValueId>> inputs)
      override;

  opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
  Write(opcua::scada::ServiceContext context,
        std::shared_ptr<const std::vector<opcua::scada::WriteValue>> inputs)
      override;

 private:
  scada::AttributeService& inner_;
};

class ViewServiceAdapter : public opcua::scada::ViewService {
 public:
  explicit ViewServiceAdapter(scada::ViewService& inner) : inner_{inner} {}

  opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::BrowseResult>>>
  Browse(opcua::scada::ServiceContext context,
         std::vector<opcua::scada::BrowseDescription> inputs) override;

  opcua::Awaitable<
      opcua::scada::StatusOr<std::vector<opcua::scada::BrowsePathResult>>>
  TranslateBrowsePaths(
      std::vector<opcua::scada::BrowsePath> inputs) override;

 private:
  scada::ViewService& inner_;
};

class MethodServiceAdapter : public opcua::scada::MethodService {
 public:
  explicit MethodServiceAdapter(scada::MethodService& inner) : inner_{inner} {}

  opcua::Awaitable<opcua::scada::Status> Call(
      opcua::scada::NodeId node_id,
      opcua::scada::NodeId method_id,
      std::vector<opcua::scada::Variant> arguments,
      opcua::scada::NodeId user_id) override;

 private:
  scada::MethodService& inner_;
};

class NodeManagementServiceAdapter
    : public opcua::scada::NodeManagementService {
 public:
  explicit NodeManagementServiceAdapter(scada::NodeManagementService& inner)
      : inner_{inner} {}

  opcua::Awaitable<
      opcua::scada::StatusOr<std::vector<opcua::scada::AddNodesResult>>>
  AddNodes(std::vector<opcua::scada::AddNodesItem> inputs) override;

  opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
  DeleteNodes(std::vector<opcua::scada::DeleteNodesItem> inputs) override;

  opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
  AddReferences(std::vector<opcua::scada::AddReferencesItem> inputs) override;

  opcua::Awaitable<opcua::scada::StatusOr<std::vector<opcua::scada::StatusCode>>>
  DeleteReferences(
      std::vector<opcua::scada::DeleteReferencesItem> inputs) override;

 private:
  scada::NodeManagementService& inner_;
};

class HistoryServiceAdapter : public opcua::scada::HistoryService {
 public:
  explicit HistoryServiceAdapter(scada::HistoryService& inner)
      : inner_{inner} {}

  opcua::Awaitable<opcua::scada::HistoryReadRawResult> HistoryReadRaw(
      opcua::scada::HistoryReadRawDetails details) override;

  opcua::Awaitable<opcua::scada::HistoryReadEventsResult> HistoryReadEvents(
      opcua::scada::NodeId node_id,
      opcua::base::Time from,
      opcua::base::Time to,
      opcua::scada::EventFilter filter) override;

 private:
  scada::HistoryService& inner_;
};

// Wraps an inner core MonitoredItemSubscription as the opcua interface.
class MonitoredItemSubscriptionAdapter
    : public opcua::scada::MonitoredItemSubscription {
 public:
  explicit MonitoredItemSubscriptionAdapter(
      std::unique_ptr<scada::MonitoredItemSubscription> inner)
      : inner_{std::move(inner)} {}

  opcua::Awaitable<std::vector<opcua::scada::MonitoredItemCreateResult>> AddItems(
      std::vector<opcua::scada::MonitoredItemCreateRequest> requests) override;

  opcua::Awaitable<std::vector<opcua::scada::Status>> RemoveItems(
      std::span<const opcua::scada::MonitoredItemId> item_ids) override;

  opcua::Awaitable<
      opcua::scada::StatusOr<std::vector<opcua::scada::MonitoredItemNotification>>>
  ReadNext(std::size_t max_count) override;

  void Close(opcua::scada::Status status) override;

 private:
  std::unique_ptr<scada::MonitoredItemSubscription> inner_;
};

class MonitoredItemServiceAdapter : public opcua::scada::MonitoredItemService {
 public:
  explicit MonitoredItemServiceAdapter(scada::MonitoredItemService& inner)
      : inner_{inner} {}

  opcua::scada::StatusOr<std::unique_ptr<opcua::scada::MonitoredItemSubscription>>
  CreateSubscription(
      opcua::scada::ServiceContext context,
      opcua::scada::MonitoredItemSubscriptionOptions options) override;

 private:
  scada::MonitoredItemService& inner_;
};

// Presents a core authenticator as the opcua interface the server session
// manager consumes.
class AuthenticatorAdapter : public opcua::scada::CoroutineAuthenticator {
 public:
  explicit AuthenticatorAdapter(
      std::shared_ptr<scada::CoroutineAuthenticator> inner)
      : inner_{std::move(inner)} {}

  opcua::Awaitable<opcua::scada::StatusOr<opcua::scada::AuthenticationResult>>
  Authenticate(opcua::scada::LocalizedText user_name,
               opcua::scada::LocalizedText password) override;

 private:
  std::shared_ptr<scada::CoroutineAuthenticator> inner_;
};

// Owns the six adapters wrapping a set of core services for opcuapp's server
// runtime. Construct from the core services and pass the member references into
// the opcua ServerRuntimeContext.
struct ServerServiceAdapters {
  ServerServiceAdapters(scada::AttributeService& attribute,
                        scada::ViewService& view,
                        scada::MethodService& method,
                        scada::NodeManagementService& node_management,
                        scada::HistoryService& history,
                        scada::MonitoredItemService& monitored_item)
      : attribute{attribute},
        view{view},
        method{method},
        node_management{node_management},
        history{history},
        monitored_item{monitored_item} {}

  AttributeServiceAdapter attribute;
  ViewServiceAdapter view;
  MethodServiceAdapter method;
  NodeManagementServiceAdapter node_management;
  HistoryServiceAdapter history;
  MonitoredItemServiceAdapter monitored_item;
};

}  // namespace opcua_bridge
