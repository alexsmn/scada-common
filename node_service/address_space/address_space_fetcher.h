#pragma once

#include "common/view_events_subscription.h"
#include "core/configuration_types.h"
#include "core/view_service.h"
#include "node_service/address_space/node_fetch_status_tracker.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_fetcher_impl.h"

#include <map>
#include <memory>

class Executor;

namespace boost::asio {
class io_context;
}

namespace scada {
class AttributeService;
}  // namespace scada

class AddressSpaceImpl;
class NodeFactory;

struct AddressSpaceFetcherContext {
  boost::asio::io_context& io_context_;
  const std::shared_ptr<Executor> executor_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  AddressSpaceImpl& address_space_;
  NodeFactory& node_factory_;

  const ViewEventsProvider view_events_provider_;

  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;

  const std::function<void(const scada::ModelChangeEvent& event)>
      model_changed_handler_;
};

class AddressSpaceFetcher
    : private AddressSpaceFetcherContext,
      private scada::ViewEvents,
      public std::enable_shared_from_this<AddressSpaceFetcher> {
 public:
  explicit AddressSpaceFetcher(AddressSpaceFetcherContext&& context);
  ~AddressSpaceFetcher();

  static std::shared_ptr<AddressSpaceFetcher> Create(
      AddressSpaceFetcherContext&& context);

  void OnChannelOpened();
  void OnChannelClosed();

  std::pair<scada::Status, NodeFetchStatus> GetNodeFetchStatus(
      const scada::NodeId& node_id) const;

  void FetchNode(const scada::NodeId& node_id,
                 const NodeFetchStatus& requested_status);

 private:
  void Init();

  NodeChildrenFetcherContext MakeNodeChildrenFetcherContext();

  void InternalFetchNode(const scada::NodeId& node_id,
                         const NodeFetchStatus& requested_status);
  void OnFetchCompleted(std::vector<scada::NodeState>&& fetched_nodes,
                        NodeFetchStatuses&& errors);

  void DeleteNode(const scada::NodeId& node_id);
  void OnChildrenFetched(const scada::NodeId& node_id,
                         scada::ReferenceDescriptions&& references);

  // scada::ViewEvents
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(
      const scada::SemanticChangeEvent& event) override;

  BoostLogger logger_{LOG_NAME("AddressSpaceFetcher")};

  std::shared_ptr<NodeFetcherImpl> node_fetcher_;
  std::shared_ptr<NodeChildrenFetcher> node_children_fetcher_;
  NodeFetchStatusTracker fetch_status_tracker_;

  bool channel_opened_ = false;

  using PostponedFetchNodes = std::map<scada::NodeId, NodeFetchStatus>;
  PostponedFetchNodes postponed_fetch_nodes_;

  std::unique_ptr<IViewEventsSubscription> view_events_subscription_ =
      view_events_provider_(*this);
};
