#pragma once

#include "base/boost_log.h"
#include "core/view_service.h"

#include <map>
#include <set>
#include <vector>

namespace scada {
class Node;
class NodeId;
struct NodeState;
}  // namespace scada

class AddressSpaceImpl;
class NodeFactory;

// Maps node id to scada::ModelChangeEvent::Verb.
using ModelChangeVerbs = std::map<scada::NodeId, uint8_t>;

class AddressSpaceUpdater {
 public:
  AddressSpaceUpdater(AddressSpaceImpl& address_space,
                      NodeFactory& node_factory);

  void UpdateNodes(std::vector<scada::NodeState>&& nodes);

  const ModelChangeVerbs& model_change_verbs() const {
    return model_change_verbs_;
  }

  const std::set<scada::NodeId>& semantic_change_node_ids() const {
    return semantic_change_node_ids_;
  }

 private:
  void AddReference(const scada::NodeId& node_id,
                    const scada::ReferenceDescription& reference);
  void DeleteReference(const scada::NodeId& node_id,
                       const scada::ReferenceDescription& reference);

  void FindDeletedReferences(const scada::Node& node,
                             const scada::ReferenceDescriptions& references,
                             scada::ReferenceDescriptions& deleted_references);

  void ReportStatistics();

  AddressSpaceImpl& address_space_;
  NodeFactory& node_factory_;

  BoostLogger logger_{LOG_NAME("AddressSpaceUpdater")};

  ModelChangeVerbs model_change_verbs_;
  std::set<scada::NodeId> semantic_change_node_ids_;

  std::vector<scada::NodeId> added_nodes_;
  std::vector<scada::NodeId> modified_nodes_;
  std::vector<std::pair<scada::NodeId, scada::ReferenceDescription>>
      added_references_;
  std::vector<std::pair<scada::NodeId, scada::ReferenceDescription>>
      deleted_references_;
};

std::vector<const scada::Node*> FindDeletedChildren(
    const scada::Node& node,
    const scada::ReferenceDescriptions& references);