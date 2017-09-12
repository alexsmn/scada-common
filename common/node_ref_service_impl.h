#pragma once

#include "base/observer_list.h"
#include "node_ref_service.h"

#include <map>

class NodeRefImpl;

struct NodeRefServiceImplContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
};

class NodeRefServiceImpl : private NodeRefServiceImplContext,
                           private scada::ViewEvents,
                           public NodeRefService {
 public:
  explicit NodeRefServiceImpl(NodeRefServiceImplContext&& context);
  ~NodeRefServiceImpl();

  NodeRef GetPartialNode(const scada::NodeId& node_id);

  NodeRef GetCachedNode(const scada::NodeId& node_id) const;

  using RequestNodeCallback = std::function<void(const scada::Status& status, const NodeRef& node)>;
  void RequestNode(const scada::NodeId& node_id, const RequestNodeCallback& callback);

  using BrowseCallback = std::function<void(const scada::Status& status, scada::ReferenceDescriptions references)>;
  void Browse(const scada::BrowseDescription& description, const BrowseCallback& callback);

  void AddObserver(NodeRefObserver& observer);
  void RemoveObserver(NodeRefObserver& observer);

  void AddNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer);
  void RemoveNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer);

 private:
  std::shared_ptr<NodeRefImpl> GetPartialNode(const scada::NodeId& node_id, const RequestNodeCallback& callback, const scada::NodeId& depended_id);

  void SetAttribute(NodeRefImpl& data, scada::AttributeId attribute_id, scada::DataValue data_value);
  void AddReference(NodeRefImpl& data, const NodeRefImplReference& reference);

  void OnReadComplete(const scada::NodeId& node_id, const scada::Status& status, std::vector<scada::DataValue> values);
  void OnBrowseComplete(const scada::NodeId& node_id, const scada::Status& status, std::vector<scada::BrowseResult> results);

  void CompletePartialNode(const scada::NodeId& node_id);

  struct PartialNode {
    unsigned pending_request_count = 0;
    std::shared_ptr<NodeRefImpl> impl;
    std::vector<NodeRefImplReference> pending_references;
    std::vector<scada::NodeId> depended_ids;
    std::vector<RequestNodeCallback> callbacks;
    bool passing = false;
  };

  bool IsNodeFetched(const std::shared_ptr<NodeRefImpl>& impl, std::vector<scada::NodeId>& fetched_node_ids);
  bool IsNodeFetchedHelper(PartialNode& partial_node, std::vector<scada::NodeId>& fetched_node_ids);

  using Observers = base::ObserverList<NodeRefObserver>;

  const Observers* GetNodeObservers(const scada::NodeId& node_id) const;

  // scada::ViewService
  virtual void OnNodeAdded(const scada::NodeId& node_id) override;
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnReferenceAdded(const scada::BrowseReference& reference) ;
  virtual void OnReferenceDeleted(const scada::BrowseReference& reference) override;
  virtual void OnNodeModified(const scada::NodeId& node_id, const scada::PropertyIds& property_ids) override;

  Observers observers_;
  std::map<scada::NodeId, Observers> node_observers_;

  std::map<scada::NodeId, std::shared_ptr<NodeRefImpl>> cached_nodes_;

  std::map<scada::NodeId, PartialNode> partial_nodes_;
};
