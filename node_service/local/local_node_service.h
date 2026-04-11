#pragma once

#include "node_service/local/local_node_model.h"
#include "node_service/node_service.h"

#include <unordered_map>

namespace boost::json {
class value;
}

// In-memory NodeService backed by a map of pre-built `LocalNodeModel`s.
// Returns the stored NodeRef for known ids and an empty NodeRef otherwise.
//
// Tree relationships (parent/children) are not tracked here — consumers
// typically pair this with a `LocalNodeServiceTree` or query the underlying
// `LocalViewService` directly.
class LocalNodeService : public NodeService {
 public:
  LocalNodeService();
  ~LocalNodeService() override;

  NodeRef AddNode(LocalNodeModel::Data data);

  // Populates nodes from the `nodes` array of a screenshot-style JSON
  // document. Each entry becomes a LocalNodeModel.
  void LoadFromJson(const boost::json::value& root);

  // NodeService
  NodeRef GetNode(const scada::NodeId& node_id) override;
  void Subscribe(NodeRefObserver& /*observer*/) const override {}
  void Unsubscribe(NodeRefObserver& /*observer*/) const override {}
  size_t GetPendingTaskCount() const override { return 0; }

 private:
  std::unordered_map<scada::NodeId, NodeRef> refs_;
};
