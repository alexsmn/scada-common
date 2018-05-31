#include "common/address_space/address_space_updater.h"

#include "address_space/address_space_impl.h"
#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/logger.h"
#include "base/strings/stringprintf.h"
#include "common/node_id_util.h"
#include "common/node_state.h"
#include "core/configuration_types.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"

namespace {

template <class T>
bool Contains(const std::set<T>& range, const T& element) {
  return range.find(element) != range.end();
}

template <class T>
bool Contains(const std::vector<T>& range, const T& element) {
  return std::find(std::begin(range), std::end(range), element) !=
         std::end(range);
}

// Type definition's properties are not merged.
// Resets |node_id| for deleted nodes.
void MergeProperties(
    scada::AddressSpace& address_space,
    std::vector<scada::NodeState>& nodes,
    std::map<scada::NodeId, scada::NodeProperties>& modified_properties) {
  std::map<scada::NodeId, scada::NodeState*> node_map;

  for (auto& node : nodes) {
    if (node.reference_type_id != scada::id::HasProperty) {
      assert(!node.node_id.is_null());
      assert(node_map.find(node.node_id) == node_map.end());
      node_map.emplace(node.node_id, &node);
    }
  }

  for (auto& node : nodes) {
    if (node.reference_type_id != scada::id::HasProperty)
      continue;

    assert(!node.node_id.is_null());
    assert(!node.parent_id.is_null());
    assert(!node.attributes.browse_name.empty());
    assert(node.references.empty());

    scada::NodeState* parent_node = nullptr;
    {
      auto i = node_map.find(node.parent_id);
      if (i != node_map.end())
        parent_node = i->second;
    }

    scada::TypeDefinition* parent_type_definition = nullptr;

    if (parent_node) {
      // Don't merge type definition's properties. Create a normal variable node
      // for them.
      if (scada::IsTypeDefinition(parent_node->node_class))
        continue;

      parent_type_definition = scada::AsTypeDefinition(
          address_space.GetNode(parent_node->type_definition_id));

    } else {
      auto* parent_node = address_space.GetNode(node.parent_id);
      assert(parent_node);
      if (!parent_node)
        continue;

      // Don't merge type definition's properties. Create a normal variable node
      // for them.
      if (scada::IsTypeDefinition(parent_node->GetNodeClass()))
        continue;

      parent_type_definition = scada::GetTypeDefinition(*parent_node);
    }

    assert(parent_type_definition);
    if (!parent_type_definition)
      continue;

    auto* property_declaration = scada::FindChildDeclaration(
        *parent_type_definition, node.attributes.browse_name.name());
    assert(property_declaration);
    if (!property_declaration)
      continue;

    if (parent_node) {
      parent_node->properties.emplace_back(property_declaration->id(),
                                           std::move(node.attributes.value));

    } else {
      auto& properties = modified_properties[node.parent_id];
      properties.emplace_back(property_declaration->id(),
                              std::move(node.attributes.value));
    }

    // Deleted.
    node.node_id = {};
  }
}

std::string FormatReference(scada::AddressSpace& address_space,
                            const scada::NodeId& node_id,
                            const scada::ReferenceDescription& reference) {
  auto& source_id = reference.forward ? node_id : reference.node_id;
  auto& target_id = reference.forward ? reference.node_id : node_id;

  auto* reference_type = address_space.GetNode(reference.reference_type_id);
  auto reference_name = reference_type
                            ? reference_type->GetBrowseName().name()
                            : NodeIdToScadaString(reference.reference_type_id);
  return base::StringPrintf("%s %s %s", NodeIdToScadaString(source_id).c_str(),
                            reference_name.c_str(),
                            NodeIdToScadaString(target_id).c_str());
}

}  // namespace

void SortNodes(std::vector<scada::NodeState>& nodes) {
  struct Visitor {
    Visitor(std::vector<scada::NodeState>& nodes)
        : nodes{nodes}, visited(nodes.size(), false) {
      for (size_t index = 0; index < nodes.size(); ++index)
        map.emplace(nodes[index].node_id, index);

      sorted_indexes.reserve(nodes.size());
      for (size_t index = 0; index < nodes.size(); ++index)
        Visit(index);

      assert(sorted_indexes.size() == nodes.size());

      std::vector<scada::NodeState> sorted_nodes(sorted_indexes.size());
      for (size_t index = 0; index < nodes.size(); ++index)
        sorted_nodes[index] = std::move(nodes[sorted_indexes[index]]);
      nodes = std::move(sorted_nodes);
    }

    void Visit(const scada::NodeId& node_id) {
      if (node_id.is_null())
        return;
      auto i = map.find(node_id);
      if (i != map.end())
        Visit(i->second);
    }

    void Visit(size_t index) {
      if (visited[index])
        return;

      visited[index] = true;

      auto& node = nodes[index];
      Visit(node.reference_type_id);
      Visit(node.parent_id);
      Visit(node.type_definition_id);
      Visit(node.attributes.data_type);

      sorted_indexes.emplace_back(index);
    }

    std::vector<scada::NodeState>& nodes;
    std::vector<bool> visited;
    std::map<scada::NodeId, size_t /*index*/> map;
    std::vector<size_t /*indexes*/> sorted_indexes;
  };

  if (nodes.size() >= 2)
    Visitor{nodes};
}

bool IsPropertyDeclaration(scada::AddressSpace& address_space,
                           const scada::NodeState& node_state) {
  if (node_state.reference_type_id != scada::id::HasProperty)
    return false;

  auto* parent = address_space.GetNode(node_state.parent_id);
  return scada::AsTypeDefinition(parent) != nullptr;
}

void UpdateNodes(AddressSpaceImpl& address_space,
                 NodeFactory& node_factory,
                 std::vector<scada::NodeState>&& nodes,
                 Logger& logger,
                 std::vector<scada::Node*>* return_added_nodes) {
  SortNodes(nodes);

  std::vector<scada::Node*> added_nodes;
  added_nodes.reserve(nodes.size());

  // Create type definitions and their properties to let MergeProperties() work.
  // Type definitions never update.
  for (auto& node_state : nodes) {
    assert(!node_state.node_id.is_null());

    if (!scada::IsTypeDefinition(node_state.node_class) &&
        !IsPropertyDeclaration(address_space, node_state))
      continue;

    auto [status, added_node] = node_factory.CreateNode(node_state);
    assert(status);
    assert(added_node);
    added_nodes.emplace_back(added_node);

    assert(address_space.GetNode(node_state.node_id));
  }

  std::map<scada::NodeId, scada::NodeProperties> modified_properties;
  // Deletes some nodes by resetting |id|.
  MergeProperties(address_space, nodes, modified_properties);

  for (auto& node_state : nodes) {
    // Skip properties.
    if (node_state.reference_type_id == scada::id::HasProperty)
      continue;

    if (auto* node = address_space.GetNode(node_state.node_id)) {
      // Skip nodes added above.
      if (std::find(added_nodes.begin(), added_nodes.end(), node) !=
          added_nodes.end())
        continue;

      logger.WriteF(
          LogSeverity::Normal,
          "ModifyNode [node_id=%s, node_class=%s, type_definition_id=%s, "
          "parent_id=%s, reference_type_id=%s]",
          NodeIdToScadaString(node_state.node_id).c_str(),
          ToString(node_state.node_class).c_str(),
          NodeIdToScadaString(node_state.type_definition_id).c_str(),
          NodeIdToScadaString(node_state.parent_id).c_str(),
          NodeIdToScadaString(node_state.reference_type_id).c_str());

      address_space.ModifyNode(node_state.node_id,
                               std::move(node_state.attributes),
                               std::move(node_state.properties));

    } else {
      auto [status, added_node] = node_factory.CreateNode(node_state);
      assert(status);
      assert(added_node);
      added_nodes.emplace_back(added_node);
    }
  }

  // Modified properties.

  for (auto& [node_id, properties] : modified_properties)
    address_space.ModifyNode(node_id, {}, std::move(properties));

  std::vector<scada::ReferenceDescription> deleted_references;

  for (auto& node_state : nodes) {
    if (node_state.reference_type_id == scada::id::HasProperty)
      continue;

    assert(!node_state.node_id.is_null());

    auto* node = address_space.GetNode(node_state.node_id);
    assert(node);
    if (!node)
      continue;

    // Deleted references.

    deleted_references.clear();

    for (const auto& ref :
         FilterReferences(node->forward_references(),
                          scada::id::NonHierarchicalReferences)) {
      // TODO: Handle type definition update.
      if (ref.type->id() == scada::id::HasTypeDefinition)
        continue;

      scada::ReferenceDescription reference{ref.type->id(), true,
                                            ref.node->id()};
      if (!Contains(node_state.references, reference))
        deleted_references.emplace_back(std::move(reference));
    }

    for (const auto& reference : deleted_references) {
      logger.WriteF(
          LogSeverity::Normal, "Reference deleted: %s",
          FormatReference(address_space, node_state.node_id, reference)
              .c_str());

      auto& source_id =
          reference.forward ? node_state.node_id : reference.node_id;
      auto& target_id =
          reference.forward ? reference.node_id : node_state.node_id;
      scada::DeleteReference(address_space, reference.reference_type_id,
                             node_state.node_id, reference.node_id);
    }

    // Added references.

    for (const auto& reference : node_state.references) {
      assert(reference.forward);
      assert(node);

      if (scada::FindReference(*node, reference))
        continue;

      logger.WriteF(
          LogSeverity::Normal, "Reference added: %s",
          FormatReference(address_space, node_state.node_id, reference)
              .c_str());

      auto& source_id =
          reference.forward ? node_state.node_id : reference.node_id;
      auto& target_id =
          reference.forward ? reference.node_id : node_state.node_id;
      scada::AddReference(address_space, reference.reference_type_id, source_id,
                          target_id);
    }
  }

  //  for (auto* node : added_nodes)
  //    address_space.NotifyNodeAdded(*node);

  if (return_added_nodes)
    *return_added_nodes = std::move(added_nodes);
}

void FindMissingTargets(AddressSpaceImpl& address_space,
                        const scada::NodeId& node_id,
                        const scada::NodeId& reference_type_id,
                        const std::set<scada::NodeId>& target_ids,
                        std::vector<scada::Node*>& missing_targets) {
  auto* node = address_space.GetNode(node_id);
  if (!node)
    return;

  for (auto* target :
       scada::GetForwardReferenceNodes(*node, reference_type_id)) {
    if (!Contains(target_ids, target->id()))
      missing_targets.emplace_back(target);
  }
}
