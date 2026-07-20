#include "node_service/v3/service_node_fetcher.h"

#include "base/boost_log.h"
#include "base/no_destructor.h"
#include "model/node_id_util.h"
#include "scada/attribute_ids.h"
#include "scada/attribute_service.h"
#include "scada/node_class.h"
#include "scada/standard_node_ids.h"
#include "scada/view_service.h"

#include <algorithm>

namespace v3 {

namespace {

BoostLogger& Logger() {
  static scada::base::NoDestructor<BoostLogger> logger{
      LOG_NAME("ServiceNodeFetcher")};
  return *logger;
}

void LogUnexpectedAttributeValue(const scada::NodeId& node_id,
                                 scada::AttributeId attribute_id,
                                 const scada::Variant& value) {
  LOG_ERROR(Logger()) << "Unexpected fetched attribute type"
                      << LOG_TAG("NodeId", NodeIdToScadaString(node_id))
                      << LOG_TAG("AttributeId", ToString(attribute_id))
                      << LOG_TAG("VariantType", ToString(value.type()))
                      << LOG_TAG("Value", ToString(value));
}

// Applies one fetched attribute to |node_state|. Mirrors
// NodeFetcherImpl::SetFetchedAttribute. Returns false only when the failure
// must fail the whole node fetch (an invalid BrowseName, or a value whose type
// does not match the attribute); other cases fill the field or are ignored.
bool ApplyFetchedAttribute(scada::NodeState& node_state,
                           scada::AttributeId attribute_id,
                           scada::Variant&& value) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass: {
      scada::Int32 node_class = 0;
      if (!value.get(node_class)) {
        LogUnexpectedAttributeValue(node_state.node_id, attribute_id, value);
        return false;
      }
      node_state.node_class = static_cast<scada::NodeClass>(node_class);
      return true;
    }

    case scada::AttributeId::BrowseName: {
      scada::QualifiedName browse_name;
      if (!value.get(browse_name) || browse_name.empty()) {
        LogUnexpectedAttributeValue(node_state.node_id, attribute_id, value);
        return false;
      }
      node_state.attributes.browse_name = std::move(browse_name);
      return true;
    }

    case scada::AttributeId::DisplayName: {
      scada::LocalizedText display_name;
      if (!value.get(display_name)) {
        LogUnexpectedAttributeValue(node_state.node_id, attribute_id, value);
        return false;
      }
      node_state.attributes.display_name = std::move(display_name);
      return true;
    }

    case scada::AttributeId::DataType: {
      scada::NodeId data_type;
      if (!value.get(data_type) || data_type.is_null()) {
        LogUnexpectedAttributeValue(node_state.node_id, attribute_id, value);
        return false;
      }
      node_state.attributes.data_type = std::move(data_type);
      return true;
    }

    case scada::AttributeId::Value:
      node_state.attributes.value = std::move(value);
      return true;

    default:
      return true;
  }
}

// Applies one browsed reference to |node_state|. Mirrors the parent/supertype
// and forward-non-hierarchical branches of NodeFetcherImpl::AddFetchedReference.
// The Aggregates/child branch is intentionally absent: v3 does not browse
// Aggregates during the node fetch — children come from FetchChildren.
void ApplyFetchedReference(scada::NodeState& node_state,
                           const scada::BrowseDescription& description,
                           scada::ReferenceDescription&& reference) {
  // Browse results come from a (possibly remote) server; malformed references
  // are external input, so log and skip rather than panic.
  if (reference.reference_type_id.is_null() || reference.node_id.is_null())
    return;

  if (description.direction == scada::BrowseDirection::Inverse) {
    // Inverse HierarchicalReferences browse: the parent (via HasComponent,
    // Organizes, ...) and the supertype (via HasSubtype).
    if (reference.forward) {
      LOG_WARNING(Logger()) << "Ignore forward reference in an inverse browse"
                            << LOG_TAG("NodeId",
                                       NodeIdToScadaString(node_state.node_id));
      return;
    }

    if (reference.reference_type_id == scada::id::HasSubtype) {
      if (!node_state.supertype_id.is_null() &&
          node_state.supertype_id != reference.node_id) {
        LOG_WARNING(Logger())
            << "Conflicting supertype in browse result"
            << LOG_TAG("NodeId", NodeIdToScadaString(node_state.node_id));
      }
      node_state.supertype_id = reference.node_id;
    } else {
      if ((!node_state.parent_id.is_null() &&
           node_state.parent_id != reference.node_id) ||
          (!node_state.reference_type_id.is_null() &&
           node_state.reference_type_id != reference.reference_type_id)) {
        LOG_WARNING(Logger())
            << "Conflicting parent in browse result"
            << LOG_TAG("NodeId", NodeIdToScadaString(node_state.node_id));
      }
      node_state.parent_id = reference.node_id;
      node_state.reference_type_id = reference.reference_type_id;
    }
    return;
  }

  // Forward NonHierarchicalReferences browse: the type definition (via
  // HasTypeDefinition) plus any other non-hierarchical references.
  if (!reference.forward) {
    LOG_WARNING(Logger()) << "Ignore inverse reference in a forward browse"
                          << LOG_TAG("NodeId",
                                     NodeIdToScadaString(node_state.node_id));
    return;
  }

  if (reference.reference_type_id == scada::id::HasTypeDefinition) {
    if (!node_state.type_definition_id.is_null() &&
        node_state.type_definition_id != reference.node_id) {
      LOG_WARNING(Logger())
          << "Conflicting type definition in browse result"
          << LOG_TAG("NodeId", NodeIdToScadaString(node_state.node_id));
    }
    node_state.type_definition_id = reference.node_id;
  } else if (std::find(node_state.references.begin(),
                       node_state.references.end(),
                       reference) == node_state.references.end()) {
    // Skip duplicate references returned by the server.
    node_state.references.push_back(std::move(reference));
  }
}

}  // namespace

ServiceNodeFetcher::ServiceNodeFetcher(ServiceNodeFetcherContext&& context)
    : ServiceNodeFetcherContext{std::move(context)} {}

Awaitable<scada::StatusOr<scada::NodeState>> ServiceNodeFetcher::FetchNode(
    const scada::NodeId& node_id) {
  scada::NodeState node_state;
  node_state.node_id = node_id;

  // Attributes. Same set as NodeFetcherImpl::GetFetchAttributes; Value is read
  // for every node (property values are needed and Value on non-Variable nodes
  // simply comes back with a bad per-value status that is skipped below).
  const std::vector<scada::ReadValueId> read_ids{
      {node_id, scada::AttributeId::BrowseName},
      {node_id, scada::AttributeId::DisplayName},
      {node_id, scada::AttributeId::NodeClass},
      {node_id, scada::AttributeId::DataType},
      {node_id, scada::AttributeId::Value},
  };

  auto read_result = co_await attribute_service_.Read(service_context_, read_ids);
  if (!read_result.ok())
    co_return read_result.status();

  auto& values = read_result.value();
  if (values.size() != read_ids.size())
    co_return scada::Status{scada::StatusCode::Bad};

  for (size_t i = 0; i < read_ids.size(); ++i) {
    const auto& read_id = read_ids[i];
    auto& data_value = values[i];

    if (scada::IsBad(data_value.status_code)) {
      // A missing BrowseName fails the node; other attributes are optional.
      if (read_id.attribute_id == scada::AttributeId::BrowseName)
        co_return scada::Status{data_value.status_code};
      continue;
    }

    if (!ApplyFetchedAttribute(node_state, read_id.attribute_id,
                               std::move(data_value.value)))
      co_return scada::Status{scada::StatusCode::Bad_WrongTypeId};
  }

  // References. Same set as NodeFetcherImpl::GetFetchReferences, minus the
  // Aggregates (children) browse: v3 fetches children via FetchChildren.
  //  - forward NonHierarchicalReferences → type definition + other refs;
  //  - inverse HierarchicalReferences    → parent and supertype.
  std::vector<scada::BrowseDescription> descriptions;
  descriptions.emplace_back(node_id, scada::BrowseDirection::Forward,
                            scada::id::NonHierarchicalReferences, true);
  if (node_id != scada::id::RootFolder) {
    descriptions.emplace_back(node_id, scada::BrowseDirection::Inverse,
                              scada::id::HierarchicalReferences, true);
  }

  auto browse_result =
      co_await view_service_.Browse(service_context_, descriptions);
  if (!browse_result.ok())
    co_return browse_result.status();

  auto& browse_results = browse_result.value();
  if (browse_results.size() != descriptions.size())
    co_return scada::Status{scada::StatusCode::Bad};

  for (size_t i = 0; i < descriptions.size(); ++i) {
    if (scada::IsBad(browse_results[i].status_code))
      continue;
    for (auto& reference : browse_results[i].references)
      ApplyFetchedReference(node_state, descriptions[i], std::move(reference));
  }

  co_return node_state;
}

Awaitable<scada::StatusOr<scada::ReferenceDescriptions>>
ServiceNodeFetcher::FetchChildren(const scada::NodeId& node_id) {
  // v3 keeps children in the model's child_references_ and resolves them via
  // GetTargets(HierarchicalReferences, ...). Unlike v2 — which splits children
  // between an Aggregates browse in the node fetch and an Organizes/HasSubtype
  // browse here — v3 does not browse Aggregates in FetchNode, so every
  // hierarchical child (components, properties, organized nodes, subtypes) must
  // come from a single forward HierarchicalReferences browse. This matches the
  // TestNodeFetcher contract the v3 model is written against.
  std::vector<scada::BrowseDescription> descriptions;
  descriptions.emplace_back(node_id, scada::BrowseDirection::Forward,
                            scada::id::HierarchicalReferences, true);

  auto browse_result =
      co_await view_service_.Browse(service_context_, descriptions);
  if (!browse_result.ok())
    co_return browse_result.status();

  scada::ReferenceDescriptions references;
  for (auto& result : browse_result.value()) {
    if (scada::IsBad(result.status_code))
      continue;
    for (auto& reference : result.references) {
      if (reference.forward && !reference.reference_type_id.is_null() &&
          !reference.node_id.is_null())
        references.push_back(std::move(reference));
    }
  }

  co_return references;
}

}  // namespace v3
