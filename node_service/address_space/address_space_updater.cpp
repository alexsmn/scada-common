#include "node_service/address_space/address_space_updater.h"

#include "address_space/address_space_impl.h"
#include "address_space/address_space_util.h"
#include "address_space/node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/object.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"
#include "base/strings/stringprintf.h"
#include "common/node_state.h"
#include "core/configuration_types.h"
#include "core/event.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "model/node_id_util.h"

#include "core/debug_util-inl.h"

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

struct TypeDefinitionPatch {
  void Patch(std::vector<scada::NodeState>& node_states) {
    for (auto& node_state : node_states) {
      if (scada::IsTypeDefinition(node_state.node_class)) {
        assert(node_state.reference_type_id != scada::id::HasSubtype);
        parents.emplace(node_state.node_id, Entry{node_state.parent_id,
                                                  node_state.reference_type_id,
                                                  node_state.supertype_id});
        node_state.parent_id = {};
        node_state.reference_type_id = {};
        node_state.supertype_id = {};
      }
    }
  }

  void Fix() {
    for (auto& [node_id, entry] : parents) {
      if (!entry.parent_id.is_null())
        scada::AddReference(address_space, entry.reference_type_id,
                            entry.parent_id, node_id);
      if (!entry.supertype_id.is_null())
        scada::AddReference(address_space, scada::id::HasSubtype,
                            entry.supertype_id, node_id);
    }
  }

  scada::AddressSpace& address_space;

  struct Entry {
    scada::NodeId parent_id;
    scada::NodeId reference_type_id;
    scada::NodeId supertype_id;
  };

  std::map<scada::NodeId /*node_id*/, Entry> parents;
};

void AddReferenceHelper(scada::AddressSpace& address_space,
                        const scada::NodeId& node_id,
                        const scada::ReferenceDescription& reference,
                        BoostLogger& logger,
                        ModelChangeVerbs& model_change_verbs) {
  assert(reference.forward);

  LOG_INFO(logger) << "Add reference"
                   << LOG_TAG("Reference", FormatReference(address_space,
                                                           node_id, reference));

  auto* reference_type =
      AsReferenceType(address_space.GetNode(reference.reference_type_id));
  if (!reference_type) {
    LOG_WARNING(logger) << "Reference type wasn't found";
    return;
  }

  auto& source_id = reference.forward ? node_id : reference.node_id;
  auto* source = address_space.GetNode(source_id);
  if (!source) {
    LOG_WARNING(logger) << "Source wasn't found";
    return;
  }

  auto& target_id = reference.forward ? reference.node_id : node_id;
  auto* target = address_space.GetNode(target_id);
  if (!target) {
    LOG_WARNING(logger) << "Target wasn't found";
    return;
  }

  scada::AddReference(address_space, reference.reference_type_id, *source,
                      *target);

  model_change_verbs[source_id] |= scada::ModelChangeEvent::ReferenceAdded;
  model_change_verbs[target_id] |= scada::ModelChangeEvent::ReferenceAdded;
}

void UpdateNodes(AddressSpaceImpl& address_space,
                 NodeFactory& node_factory,
                 std::vector<scada::NodeState>&& nodes,
                 BoostLogger& logger,
                 ModelChangeVerbs& model_change_verbs) {
  TypeDefinitionPatch type_definition_patch{address_space};
  type_definition_patch.Patch(nodes);

  SortNodesHierarchically(nodes);

  for (auto& node_state : nodes) {
    if (auto* node = address_space.GetNode(node_state.node_id)) {
      LOG_INFO(logger) << "Node modified"
                       << LOG_TAG("Node", ToString(node_state));

      address_space.ModifyNode(node_state.node_id,
                               std::move(node_state.attributes),
                               std::move(node_state.properties));

    } else {
      auto [status, added_node] = node_factory.CreateNode(node_state);
      if (!status) {
        LOG_WARNING(logger) << "Can't update node"
                            << LOG_TAG("NodeId", ToString(node_state.node_id));
        continue;
      }

      model_change_verbs[node_state.node_id] |=
          scada::ModelChangeEvent::NodeAdded |
          scada::ModelChangeEvent::ReferenceAdded;

      if (!node_state.parent_id.is_null()) {
        model_change_verbs[node_state.parent_id] |=
            scada::ModelChangeEvent::ReferenceAdded;
      }
    }
  }

  std::vector<scada::ReferenceDescription> added_references;
  std::vector<scada::ReferenceDescription> deleted_references;

  for (auto& node_state : nodes) {
    assert(!node_state.node_id.is_null());

    auto* node = address_space.GetNode(node_state.node_id);
    if (!node) {
      LOG_WARNING(logger) << "Node wasn't found"
                          << LOG_TAG("NodeId",
                                     NodeIdToScadaString(node_state.node_id));
      continue;
    }

    deleted_references.clear();

    // Deleted references.

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
      LOG_INFO(logger) << "Reference deleted"
                       << LOG_TAG(
                              "Reference",
                              FormatReference(address_space, node_state.node_id,
                                              reference));

      auto& source_id =
          reference.forward ? node_state.node_id : reference.node_id;
      auto& target_id =
          reference.forward ? reference.node_id : node_state.node_id;
      scada::DeleteReference(address_space, reference.reference_type_id,
                             node_state.node_id, reference.node_id);

      model_change_verbs[source_id] |=
          scada::ModelChangeEvent::ReferenceDeleted;
      model_change_verbs[target_id] |=
          scada::ModelChangeEvent::ReferenceDeleted;
    }

    // Added references.

    assert(node);
    for (const auto& reference : node_state.references) {
      if (!scada::FindReference(*node, reference)) {
        AddReferenceHelper(address_space, node_state.node_id, reference, logger,
                           model_change_verbs);
      }
    }
  }

  type_definition_patch.Fix();
}

std::vector<const scada::Node*> FindDeletedChildren(
    const scada::Node& node,
    const scada::ReferenceDescriptions& references) {
  std::vector<scada::NodeId> new_child_ids;
  new_child_ids.reserve(references.size());
  for (const auto& reference : references) {
    assert(reference.forward);
    new_child_ids.emplace_back(reference.node_id);
  }
  std::sort(new_child_ids.begin(), new_child_ids.end());

  std::vector<const scada::Node*> deleted_children;
  for (auto* child : scada::GetOrganizes(node)) {
    if (!std::binary_search(new_child_ids.begin(), new_child_ids.end(),
                            child->id())) {
      deleted_children.emplace_back(child);
    }
  }
  return deleted_children;
}