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

using ModelChangeVerbs = std::map<scada::NodeId, uint8_t>;

// |model_change_verbs| maps node id to scada::ModelChangeEvent::Verb.
void UpdateNodes(AddressSpaceImpl& address_space,
                 NodeFactory& node_factory,
                 std::vector<scada::NodeState>&& nodes,
                 BoostLogger& logger,
                 ModelChangeVerbs& model_change_verbs);

void SortNodes(std::vector<scada::NodeState>& nodes);

std::vector<const scada::Node*> FindDeletedChildren(
    const scada::Node& node,
    const scada::ReferenceDescriptions& references);