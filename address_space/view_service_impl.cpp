#include "view_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

SyncViewServiceImpl::SyncViewServiceImpl(ViewServiceImplContext&& context)
    : ViewServiceImplContext{std::move(context)} {}

ViewServiceImpl::ViewServiceImpl(SyncViewService& sync_view_service)
    : sync_view_service_{sync_view_service} {}

scada::BrowseResult SyncViewServiceImpl::Browse(
    const scada::BrowseDescription& description) {
  std::string_view nested_name;
  auto* node = GetNestedNode(address_space_, description.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId};

  if (nested_name.empty())
    return BrowseNode(*node, description);

  return BrowseProperty(*node, nested_name, description);
}

scada::BrowseResult SyncViewServiceImpl::BrowseNode(
    const scada::Node& node,
    const scada::BrowseDescription& description) {
  scada::BrowseResult result;
  result.status_code = scada::StatusCode::Good;

  if (description.direction == scada::BrowseDirection::Forward ||
      description.direction == scada::BrowseDirection::Both) {
    for (const auto& ref : scada::FilterReferences(
             node.forward_references(), description.reference_type_id,
             description.include_subtypes)) {
      assert(!ref.type->id().is_null());
      assert(!ref.node->id().is_null());
      result.references.push_back({ref.type->id(), true, ref.node->id()});
    }
  }

  if (description.direction == scada::BrowseDirection::Inverse ||
      description.direction == scada::BrowseDirection::Both) {
    for (const auto& ref : scada::FilterReferences(
             node.inverse_references(), description.reference_type_id,
             description.include_subtypes)) {
      assert(!ref.type->id().is_null());
      assert(!ref.node->id().is_null());
      result.references.push_back({ref.type->id(), false, ref.node->id()});
    }
  }

  return result;
}

scada::BrowseResult SyncViewServiceImpl::BrowseProperty(
    const scada::Node& node,
    std::string_view nested_name,
    const scada::BrowseDescription& description) {
  scada::BrowseResult result;

  if (!description.include_subtypes) {
    assert(false);
    result.status_code = scada::StatusCode::Bad;
    return result;
  }

  auto* type_definition = node.type_definition();
  auto* prop_decl = type_definition
                        ? scada::AsVariable(scada::FindChildDeclaration(
                              *type_definition, nested_name))
                        : nullptr;
  if (!prop_decl) {
    result.status_code = scada::StatusCode::Bad_WrongNodeId;
    return result;
  }

  if (description.direction == scada::BrowseDirection::Forward ||
      description.direction == scada::BrowseDirection::Both) {
    if (scada::IsSubtypeOf(address_space_, scada::id::HasTypeDefinition,
                           description.reference_type_id))
      result.references.push_back(
          {scada::id::HasTypeDefinition, true, scada::id::PropertyType});
  }

  if (description.direction == scada::BrowseDirection::Inverse ||
      description.direction == scada::BrowseDirection::Both) {
    if (scada::IsSubtypeOf(address_space_, scada::id::HasProperty,
                           description.reference_type_id))
      result.references.push_back({scada::id::HasProperty, false, node.id()});
  }

  return result;
}

void ViewServiceImpl::Browse(
    const std::vector<scada::BrowseDescription>& descriptions,
    const scada::BrowseCallback& callback) {
  std::vector<scada::BrowseResult> results(descriptions.size());

  for (size_t index = 0; index < descriptions.size(); ++index)
    results[index] = sync_view_service_.Browse(descriptions[index]);

  callback(scada::StatusCode::Good, std::move(results));
}

void ViewServiceImpl::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  assert(false);
  callback(scada::StatusCode::Bad, {});
}
