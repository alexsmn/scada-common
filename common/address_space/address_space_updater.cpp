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

struct TypeDefinitionPatch {
  void Patch(std::vector<scada::NodeState>& node_states) {
    for (auto& node_state : node_states) {
      if (scada::IsTypeDefinition(node_state.node_class) &&
          !node_state.parent_id.is_null()) {
        parents.emplace(
            node_state.node_id,
            std::make_pair(node_state.parent_id, node_state.reference_type_id));
        node_state.parent_id = {};
        node_state.reference_type_id = {};
      }
    }
  }

  void Fix() {
    for (auto& [node_id, p] : parents)
      scada::AddReference(address_space, p.second, p.first, node_id);
  }

  scada::AddressSpace& address_space;

  std::map<scada::NodeId /*node_id*/,
           std::pair<scada::NodeId /*parent_id*/,
                     scada::NodeId /*reference_type_id*/>>
      parents;
};

void UpdateNodes(AddressSpaceImpl& address_space,
                 NodeFactory& node_factory,
                 std::vector<scada::NodeState>&& nodes,
                 Logger& logger,
                 std::vector<scada::Node*>* return_added_nodes) {
  TypeDefinitionPatch type_definition_patch{address_space};
  type_definition_patch.Patch(nodes);

  SortNodes(nodes);

  std::vector<scada::Node*> added_nodes;
  added_nodes.reserve(nodes.size());

  for (auto& node_state : nodes) {
    if (auto* node = address_space.GetNode(node_state.node_id)) {
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

  std::vector<scada::ReferenceDescription> deleted_references;

  for (auto& node_state : nodes) {
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

  type_definition_patch.Fix();

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
