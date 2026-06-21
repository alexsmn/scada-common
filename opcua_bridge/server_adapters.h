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

class AttributeServiceAdapter : public opcua::AttributeService {
 public:
  explicit AttributeServiceAdapter(scada::AttributeService& inner)
      : inner_{inner} {}

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::DataValue>>>
  Read(opcua::ServiceContext context,
       std::shared_ptr<const std::vector<opcua::ReadValueId>> inputs)
      override;

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
  Write(opcua::ServiceContext context,
        std::shared_ptr<const std::vector<opcua::WriteValue>> inputs)
      override;

 private:
  scada::AttributeService& inner_;
};

class ViewServiceAdapter : public opcua::ViewService {
 public:
  explicit ViewServiceAdapter(scada::ViewService& inner) : inner_{inner} {}

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::BrowseResult>>>
  Browse(opcua::ServiceContext context,
         std::vector<opcua::BrowseDescription> inputs) override;

  opcua::Awaitable<
      opcua::StatusOr<std::vector<opcua::BrowsePathResult>>>
  TranslateBrowsePaths(
      std::vector<opcua::BrowsePath> inputs) override;

 private:
  scada::ViewService& inner_;
};

class MethodServiceAdapter : public opcua::MethodService {
 public:
  explicit MethodServiceAdapter(scada::MethodService& inner) : inner_{inner} {}

  opcua::Awaitable<opcua::Status> Call(
      opcua::NodeId node_id,
      opcua::NodeId method_id,
      std::vector<opcua::Variant> arguments,
      opcua::NodeId user_id) override;

 private:
  scada::MethodService& inner_;
};

class NodeManagementServiceAdapter
    : public opcua::NodeManagementService {
 public:
  explicit NodeManagementServiceAdapter(scada::NodeManagementService& inner)
      : inner_{inner} {}

  opcua::Awaitable<
      opcua::StatusOr<std::vector<opcua::AddNodesResult>>>
  AddNodes(std::vector<opcua::AddNodesItem> inputs) override;

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
  DeleteNodes(std::vector<opcua::DeleteNodesItem> inputs) override;

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
  AddReferences(std::vector<opcua::AddReferencesItem> inputs) override;

  opcua::Awaitable<opcua::StatusOr<std::vector<opcua::StatusCode>>>
  DeleteReferences(
      std::vector<opcua::DeleteReferencesItem> inputs) override;

 private:
  scada::NodeManagementService& inner_;
};

class HistoryServiceAdapter : public opcua::HistoryService {
 public:
  explicit HistoryServiceAdapter(scada::HistoryService& inner)
      : inner_{inner} {}

  opcua::Awaitable<opcua::HistoryReadRawResult> HistoryReadRaw(
      opcua::HistoryReadRawDetails details) override;

  opcua::Awaitable<opcua::HistoryReadEventsResult> HistoryReadEvents(
      opcua::NodeId node_id,
      opcua::base::Time from,
      opcua::base::Time to,
      opcua::EventFilter filter) override;

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

  opcua::Awaitable<std::vector<opcua::MonitoredItemCreateResult>> AddItems(
      std::vector<opcua::MonitoredItemCreateRequest> requests) override;

  opcua::Awaitable<std::vector<opcua::Status>> RemoveItems(
      std::span<const opcua::scada::MonitoredItemId> item_ids) override;

  opcua::Awaitable<
      opcua::StatusOr<std::vector<opcua::scada::MonitoredItemNotification>>>
  ReadNext(std::size_t max_count) override;

  void Close(opcua::Status status) override;

 private:
  std::unique_ptr<scada::MonitoredItemSubscription> inner_;
};

class MonitoredItemServiceAdapter : public opcua::scada::MonitoredItemService {
 public:
  explicit MonitoredItemServiceAdapter(scada::MonitoredItemService& inner)
      : inner_{inner} {}

  opcua::StatusOr<std::unique_ptr<opcua::scada::MonitoredItemSubscription>>
  CreateSubscription(
      opcua::ServiceContext context,
      opcua::scada::MonitoredItemSubscriptionOptions options) override;

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

  opcua::Awaitable<opcua::StatusOr<opcua::AuthenticationResult>>
  Authenticate(opcua::LocalizedText user_name,
               opcua::LocalizedText password) override;

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
