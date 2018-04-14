#pragma once

#include <set>
#include <vector>

namespace scada {
class Node;
class NodeId;
struct NodeState;
}  // namespace scada

class ConfigurationImpl;
class Logger;
class NodeFactory;

void UpdateNodes(ConfigurationImpl& address_space,
                 NodeFactory& node_factory,
                 std::vector<scada::NodeState>&& nodes,
                 Logger& logger,
                 std::vector<scada::Node*>* added_nodes = nullptr);

void SortNodes(std::vector<scada::NodeState>& nodes);

void FindMissingTargets(ConfigurationImpl& address_space,
                        const scada::NodeId& node_id,
                        const scada::NodeId& reference_type_id,
                        const std::set<scada::NodeId>& target_ids,
                        std::vector<scada::Node*>& missing_targets);
