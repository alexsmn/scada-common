#pragma once

#include <functional>

#include "base/observer_list.h"
#include "common/node_ref.h"
#include "core/status.h"
#include "common/node_ref_cache.h"
#include "core/view_service.h"

namespace scada {
class AttributeService;
}

class Logger;
class NodeRefObserver;

struct NodeRefServiceContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
};

class NodeRefService : private NodeRefServiceContext,
                       private scada::ViewEvents {
 public:
  explicit NodeRefService(NodeRefServiceContext&& context);
  ~NodeRefService();

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
  NodeRef GetPartialNode(const scada::NodeId& node_id, const RequestNodeCallback& callback, const scada::NodeId& depended_id);

  void SetAttribute(NodeRefData& data, scada::AttributeId attribute_id, scada::DataValue data_value);
  void AddReference(NodeRefData& data, const NodeRef::Reference& reference);

  void OnReadComplete(const scada::NodeId& node_id, const scada::Status& status, std::vector<scada::DataValue> values);
  void OnBrowseComplete(const scada::NodeId& node_id, const scada::Status& status, std::vector<scada::BrowseResult> results);

  void CompletePartialNode(const scada::NodeId& node_id);

  struct PartialNode {
    unsigned pending_request_count = 0;
    NodeRef node;
    std::vector<NodeRef::Reference> pending_references;
    std::vector<scada::NodeId> depended_ids;
    std::vector<RequestNodeCallback> callbacks;
    bool passing = false;
  };

  bool IsNodeFetched(std::shared_ptr<NodeRefData> data, std::vector<scada::NodeId>& fetched_node_ids);
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

  NodeRefCache node_cache_;

  std::map<scada::NodeId, PartialNode> partial_nodes_;
};

// Callback = void(const scada::Status& status, const scada::ReferenceDescription& reference)
template<class Callback>
inline void BrowseReference(NodeRefService& service, const scada::BrowseDescription& description, const Callback& callback) {
  service.Browse(description, [callback](const scada::Status& status, const scada::ReferenceDescriptions& references) {
    if (!status) {
      assert(references.empty());
      callback(status, {});
    } else if (references.empty()) {
      assert(references.empty());
      callback(scada::StatusCode::Bad_WrongReferenceId, {});
    } else {
      // FIXME:
      // assert(references.size() == 1);
      callback(status, references.front());
    }
  });
}

// Callback = void(const scada::Status& status, const NodeRef& node)
template<class Callback>
inline void BrowseNode(NodeRefService& service, const scada::BrowseDescription& description, const Callback& callback) {
  BrowseReference(service, description,
    [&service, callback](const scada::Status& status, const scada::ReferenceDescription& reference) {
      if (!status) {
        callback(status, nullptr);
        return;
      }
      service.RequestNode(reference.node_id, [callback](const scada::Status& status, const NodeRef& node) {
        callback(status, node);
      });
    });
}

// Callback = void(const scada::NodeId& parent_id)
template<class Callback>
inline void BrowseParentId(NodeRefService& service, const scada::NodeId& node_id, const scada::NodeId& reference_type_id, const Callback& callback) {
  BrowseReference(service, {node_id, scada::BrowseDirection::Inverse, reference_type_id, true},
      [callback](const scada::Status& status, const scada::ReferenceDescription& reference) {
        callback(reference.node_id);
      });
}

// Callback = void(const scada::Status& status, const NodeRef& node)
template<class Callback>
inline void BrowseParent(NodeRefService& service, const scada::NodeId& node_id, const scada::NodeId& reference_type_id, const Callback& callback) {
  BrowseNode(service, {node_id, scada::BrowseDirection::Inverse, reference_type_id, true}, callback);
}

// Callback = void(std::vector<scada::Status> statuses, std::vector<NodeRef> nodes)
template<class Callback>
inline void RequestNodes(NodeRefService& service, const std::vector<scada::NodeId>& node_ids, const Callback& callback) {
  struct Result {
    Result(size_t count, const Callback& callback)
        : statuses{count, scada::StatusCode::Bad},
          nodes{count},
          callback{callback} {
    }

    ~Result() { callback(std::move(statuses), std::move(nodes)); }

    std::vector<scada::Status> statuses;
    std::vector<NodeRef> nodes;
    const Callback callback;
  };

  auto result = std::make_shared<Result>(node_ids.size(), callback);
  for (size_t i = 0; i < node_ids.size(); ++i) {
    service.RequestNode(node_ids[i], [result, i](const scada::Status& status, const NodeRef& node) {
      result->statuses[i] = status;
      result->nodes[i] = node;
    });
  }
}

// Callback = void(const scada::Status& status, std::vector<NodeRef> nodes)
template<class Callback>
inline void BrowseNodes(NodeRefService& service,const  scada::BrowseDescription& description, const Callback& callback) {
  service.Browse(description, [&service, callback](const scada::Status& status, const scada::ReferenceDescriptions& references) {
    if (!status) {
      callback(status, {});
      return;
    }

    std::vector<scada::NodeId> node_ids;
    node_ids.reserve(references.size());
    for (auto& ref : references)
      node_ids.emplace_back(ref.node_id);

    RequestNodes(service, node_ids,
        [callback](const std::vector<scada::Status>&, std::vector<NodeRef> nodes) {
          nodes.erase(std::remove(nodes.begin(), nodes.end(), nullptr), nodes.end());
          callback(scada::StatusCode::Good, std::move(nodes));
        });
  });
}
