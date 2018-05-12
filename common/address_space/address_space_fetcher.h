#pragma once

#include "common/address_space/node_fetch_status_tracker.h"
#include "common/node_children_fetcher.h"
#include "common/node_fetcher_impl.h"
#include "core/configuration_types.h"
#include "core/view_service.h"

#include <map>
#include <memory>

namespace scada {
class AttributeService;
}  // namespace scada

class AddressSpaceImpl;
class Logger;
class NodeFactory;

struct AddressSpaceFetcherContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  AddressSpaceImpl& address_space_;
  NodeFactory& node_factory_;

  const NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;
  const std::function<void(const scada::ModelChangeEvent& event)>
      model_changed_handler_;
};

class AddressSpaceFetcher : private AddressSpaceFetcherContext,
                            private scada::ViewEvents {
 public:
  explicit AddressSpaceFetcher(AddressSpaceFetcherContext&& context);
  ~AddressSpaceFetcher();

  void OnChannelOpened();
  void OnChannelClosed();

  std::pair<scada::Status, NodeFetchStatus> GetNodeFetchStatus(
      const scada::NodeId& node_id) const;

  void FetchNode(const scada::NodeId& node_id,
                 const NodeFetchStatus& requested_status);

 private:
  NodeFetcherImplContext MakeNodeFetcherImplContext();
  NodeChildrenFetcherContext MakeNodeChildrenFetcherContext();

  void InternalFetchNode(const scada::NodeId& node_id,
                         const NodeFetchStatus& requested_status);

  // scada::ViewEvents
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  NodeFetcherImpl node_fetcher_;
  NodeChildrenFetcher node_children_fetcher_;
  NodeFetchStatusTracker fetch_status_tracker_;

  bool channel_opened_ = false;

  using PostponedFetchNodes = std::map<scada::NodeId, NodeFetchStatus>;
  PostponedFetchNodes postponed_fetch_nodes_;
};
