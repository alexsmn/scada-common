#include "common/address_space_updater.h"

#include "base/logger.h"
#include "common/node_id_util.h"
#include "common/node_state.h"
#include "core/configuration_impl.h"
#include "core/configuration_types.h"
#include "core/node_class.h"
#include "core/node_utils.h"
#include "core/object.h"
#include "core/standard_node_ids.h"
#include "core/type_definition.h"
#include "core/variable.h"

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

void MergeProperties(
    scada::Configuration& address_space,
    std::vector<scada::NodeState>& nodes,
    std::map<scada::NodeId, scada::NodeProperties>& modified_properties) {
  scada::NodeId parent_id;
  scada::NodeId property_declaration_id;

  std::map<scada::NodeId, scada::NodeState*> node_map;

  for (auto& node : nodes) {
    if (node.reference_type_id != scada::id::HasProperty)
      node_map.emplace(node.node_id, &node);
  }

  for (auto& node : nodes) {
    if (node.reference_type_id != scada::id::HasProperty)
      continue;

    assert(!node.parent_id.is_null());
    assert(!node.attributes.browse_name.empty());

    scada::NodeState* parent_node = nullptr;
    {
      auto i = node_map.find(node.parent_id);
      if (i != node_map.end())
        parent_node = i->second;
    }

    scada::TypeDefinition* type_definition = nullptr;

    if (parent_node) {
      assert(!scada::IsTypeDefinition(parent_node->node_class));
      type_definition = scada::AsTypeDefinition(
          address_space.GetNode(parent_node->type_definition_id));

    } else {
      auto* parent_node = address_space.GetNode(node.parent_id);
      assert(parent_node);
      if (!parent_node)
        continue;

      type_definition = scada::GetTypeDefinition(*parent_node);
    }

    assert(type_definition);
    if (!type_definition)
      continue;

    auto* property_declaration = scada::FindChildDeclaration(
        *type_definition, node.attributes.browse_name.name());
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
  }
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
      auto i = map.find(node_id);
      if (i != map.end())
        Visit(i->second);
    }

    void Visit(size_t index) {
      if (visited[index])
        return;

      visited[index] = true;

      auto& node = nodes[index];
      Visit(nodes[index].parent_id);

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

// Creates new node. If |id| is null, create new id.
scada::Node* CreateNode(ConfigurationImpl& address_space,
                        const scada::NodeState& data,
                        Logger& logger) {
  logger.WriteF(LogSeverity::Normal,
                "CreateNode [node_id=%s, node_class=%s, type_definition_id=%s, "
                "parent_id=%s, reference_type_id=%s",
                NodeIdToScadaString(data.node_id).c_str(),
                ToString(data.node_class).c_str(),
                NodeIdToScadaString(data.type_definition_id).c_str(),
                NodeIdToScadaString(data.parent_id).c_str(),
                NodeIdToScadaString(data.reference_type_id).c_str());

  if (data.node_id != scada::id::RootFolder) {
    assert(!data.parent_id.is_null());
    assert(!data.reference_type_id.is_null());
  }

  if (address_space.GetNode(data.node_id))
    throw scada::Status(scada::StatusCode::Bad_DuplicateNodeId);

  auto* type_definition =
      AsTypeDefinition(address_space.GetNode(data.type_definition_id));
  if (!type_definition)
    throw scada::Status(scada::StatusCode::Bad_WrongTypeId);

  std::unique_ptr<scada::Node> node;
  if (data.node_class == scada::NodeClass::Object) {
    if (!type_definition ||
        type_definition->GetNodeClass() != scada::NodeClass::ObjectType)
      return nullptr;
    node = std::make_unique<scada::GenericObject>();

  } else if (data.node_class == scada::NodeClass::Variable) {
    auto* variable_type = scada::AsVariableType(type_definition);
    if (!variable_type)
      return nullptr;
    auto* data_type =
        scada::AsDataType(address_space.GetNode(data.attributes.data_type));
    if (!data_type)
      return nullptr;
    node = std::make_unique<scada::GenericVariable>(*data_type);

  } else
    return nullptr;

  node->set_id(std::move(data.node_id));

  scada::AddReference(address_space, scada::id::HasTypeDefinition, *node,
                      *type_definition);

  if (!data.attributes.browse_name.empty())
    node->SetBrowseName(data.attributes.browse_name);
  if (!data.attributes.display_name.empty())
    node->SetDisplayName(data.attributes.display_name);

  for (auto& prop : data.properties) {
    /*logger.WriteF(LogSeverity::Normal, "Property %s[%s] = %s",
                  NodeIdToScadaString(data.id).c_str(),
                  NodeIdToScadaString(prop.first).c_str(),
                  prop.second.get_or(std::string{"(unknown)"}).c_str());*/
    auto result =
        scada::SetPropertyValue(*node, prop.first, std::move(prop.second));
    if (!result)
      return nullptr;
  }

  auto* node_ptr = node.get();

  address_space.AddStaticNode(std::move(node));

  if (!data.parent_id.is_null()) {
    auto* parent = address_space.GetNode(data.parent_id);
    if (!parent)
      throw scada::Status(scada::StatusCode::Bad_WrongParentId);

    auto* reference_type =
        AsReferenceType(address_space.GetNode(data.reference_type_id));
    if (!reference_type)
      throw scada::Status(scada::StatusCode::Bad_WrongReferenceId);

    scada::AddReference(*reference_type, *parent, *node_ptr);
  }

  return node_ptr;
}

void UpdateNodes(ConfigurationImpl& address_space,
                 std::vector<scada::NodeState>&& nodes,
                 Logger& logger,
                 std::vector<scada::Node*>* return_added_nodes) {
  SortNodes(nodes);

  std::map<scada::NodeId, scada::NodeProperties> modified_properties;
  MergeProperties(address_space, nodes, modified_properties);

  std::vector<scada::Node*> added_nodes;
  added_nodes.reserve(nodes.size());

  for (auto& node_state : nodes) {
    if (node_state.reference_type_id == scada::id::HasProperty)
      continue;

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
      auto* added_node = CreateNode(address_space, node_state, logger);
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

    auto* node = address_space.GetNode(node_state.node_id);
    if (!node)
      continue;

    // Deleted references.

    deleted_references.clear();

    for (auto& ref : FilterReferences(node->forward_references(),
                                      scada::id::NonHierarchicalReferences)) {
      // TODO: Handle type definition update.
      if (ref.type->id() == scada::id::HasTypeDefinition)
        continue;

      scada::ReferenceDescription reference{ref.type->id(), true,
                                            ref.node->id()};
      if (!Contains(node_state.references, reference))
        deleted_references.emplace_back(std::move(reference));
    }

    for (auto& reference : deleted_references) {
      auto& source_id =
          reference.forward ? node_state.node_id : reference.node_id;
      auto& target_id =
          reference.forward ? reference.node_id : node_state.node_id;
      logger.WriteF(LogSeverity::Normal, "Reference deleted: %s --%s-> %s",
                    NodeIdToScadaString(source_id).c_str(),
                    NodeIdToScadaString(reference.reference_type_id).c_str(),
                    NodeIdToScadaString(target_id).c_str());

      scada::DeleteReference(address_space, reference.reference_type_id,
                             node_state.node_id, reference.node_id);
    }

    // Added references.

    for (auto& reference : node_state.references) {
      assert(reference.forward);
      assert(node);
      if (scada::FindReference(*node, reference))
        continue;

      auto& source_id =
          reference.forward ? node_state.node_id : reference.node_id;
      auto& target_id =
          reference.forward ? reference.node_id : node_state.node_id;
      logger.WriteF(LogSeverity::Normal, "Reference added: %s --%s-> %s",
                    NodeIdToScadaString(source_id).c_str(),
                    NodeIdToScadaString(reference.reference_type_id).c_str(),
                    NodeIdToScadaString(target_id).c_str());

      scada::AddReference(address_space, reference.reference_type_id, source_id,
                          target_id);
    }
  }

  //  for (auto* node : added_nodes)
  //    address_space.NotifyNodeAdded(*node);

  if (return_added_nodes)
    *return_added_nodes = std::move(added_nodes);
}

void FindMissingTargets(ConfigurationImpl& address_space,
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
