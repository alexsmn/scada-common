#pragma once

#include "common/view_events_subscription.h"
#include "common/node_state.h"
#include "scada/view_service.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/v1/address_space_fetcher.h"
#include "node_service/v1/node_fetch_status_tracker.h"

#include <map>
#include <memory>

class Executor;

namespace scada {
class AttributeService;
}  // namespace scada

class AddressSpaceImpl;
class NodeFactory;

namespace v1 {

struct AddressSpaceFetcherImplContext {
  const std::shared_ptr<Executor> executor_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  AddressSpaceImpl& address_space_;
  NodeFactory& node_factory_;

  const ViewEventsProvider view_events_provider_;

  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;

  const std::function<void(const scada::ModelChangeEvent& event)>
      model_changed_handler_;

  const std::function<void(const scada::SemanticChangeEvent& event)>
      semantic_changed_handler_;
};

class AddressSpaceFetcherImpl
    : private AddressSpaceFetcherImplContext,
      private scada::ViewEvents,
      public AddressSpaceFetcher,
      public std::enable_shared_from_this<AddressSpaceFetcherImpl> {
 public:
  explicit AddressSpaceFetcherImpl(AddressSpaceFetcherImplContext&& context);
  ~AddressSpaceFetcherImpl();

  static std::shared_ptr<AddressSpaceFetcherImpl> Create(
      AddressSpaceFetcherImplContext&& context);

  // AddressSpaceFetcher
  virtual void OnChannelOpened() override;
  virtual void OnChannelClosed() override;
  virtual std::pair<scada::Status, NodeFetchStatus> GetNodeFetchStatus(
      const scada::NodeId& node_id) const override;
  virtual void FetchNode(const scada::NodeId& node_id,
                         const NodeFetchStatus& requested_status) override;
  virtual size_t GetPendingTaskCount() const override;

 private:
  void Init();

  NodeChildrenFetcherContext MakeNodeChildrenFetcherContext();

  void InternalFetchNode(const scada::NodeId& node_id,
                         const NodeFetchStatus& requested_status);
  void OnFetchCompleted(FetchCompletedResult&& fetch_completed_result);

  void DeleteNode(const scada::NodeId& node_id);
  void OnChildrenFetched(const scada::NodeId& node_id,
                         scada::ReferenceDescriptions&& references);

  void FillMissingParent(scada::NodeState& node_state);

  // scada::ViewEvents
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(
      const scada::SemanticChangeEvent& event) override;

  BoostLogger logger_{LOG_NAME("AddressSpaceFetcherImpl")};

  std::shared_ptr<NodeFetcherImpl> node_fetcher_;
  std::shared_ptr<NodeChildrenFetcher> node_children_fetcher_;
  NodeFetchStatusTracker node_fetch_status_tracker_;

  bool channel_opened_ = false;

  using PostponedFetchNodes = std::map<scada::NodeId, NodeFetchStatus>;
  PostponedFetchNodes postponed_fetch_nodes_;

  std::unique_ptr<IViewEventsSubscription> view_events_subscription_ =
      view_events_provider_(*this);
};

}  // namespace v1
