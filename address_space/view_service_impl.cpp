#include "view_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "base/range_util.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

namespace {

const scada::Node* TranslateBrowsePathHelper2(
    const scada::Node& node,
    const scada::RelativePathElement& element) {
  const auto& references =
      element.inverse ? node.inverse_references() : node.forward_references();
  std::vector<scada::Node*> result;
  for (const auto& ref : scada::FilterReferences(
           references, element.reference_type_id, element.include_subtypes)) {
    // TODO: Compare namespace indexes.
    if (ref.node->GetBrowseName().name() == element.target_name.name())
      return ref.node;
  }
  return nullptr;
}

scada::BrowsePathResult TranslateBrowsePathHelper(
    const scada::Node& node,
    const scada::RelativePath& relative_path) {
  if (relative_path.empty())
    return {scada::StatusCode::Bad_NothingToDo, {}};

  auto* current_node = &node;
  size_t i = 0;
  for (; i < relative_path.size(); ++i) {
    auto* next_node =
        TranslateBrowsePathHelper2(*current_node, relative_path[i]);
    if (!next_node)
      return {scada::StatusCode::Bad_BrowseNameInvalid, {}};
    current_node = next_node;
  }

  return {scada::StatusCode::Good, {{current_node->id(), i}}};
}

}  // namespace

// SyncViewServiceImpl

SyncViewServiceImpl::SyncViewServiceImpl(ViewServiceImplContext&& context)
    : ViewServiceImplContext{std::move(context)} {}

ViewServiceImpl::ViewServiceImpl(SyncViewService& sync_view_service)
    : sync_view_service_{sync_view_service} {}

std::vector<scada::BrowseResult> SyncViewServiceImpl::Browse(
    base::span<const scada::BrowseDescription> inputs) {
  return inputs |
         boost::adaptors::transformed(
             [this](const scada::BrowseDescription& input) {
               return BrowseOne(input);
             }) |
         to_vector;
}

scada::BrowseResult SyncViewServiceImpl::BrowseOne(
    const scada::BrowseDescription& input) {
  std::string_view nested_name;
  auto* node = GetNestedNode(address_space_, input.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId};

  if (nested_name.empty())
    return BrowseNode(*node, input);

  return BrowseProperty(*node, nested_name, input);
}

std::vector<scada::BrowsePathResult> SyncViewServiceImpl::TranslateBrowsePaths(
    base::span<const scada::BrowsePath> inputs) {
  return inputs |
         boost::adaptors::transformed([this](const scada::BrowsePath& input) {
           return TranslateBrowsePath(input);
         }) |
         to_vector;
}

scada::BrowsePathResult SyncViewServiceImpl::TranslateBrowsePath(
    const scada::BrowsePath& input) {
  auto* node = address_space_.GetNode(input.node_id);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId};

  return TranslateBrowsePathHelper(*node, input.relative_path);
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
  auto results = sync_view_service_.Browse(descriptions);
  callback(scada::StatusCode::Good, std::move(results));
}

void ViewServiceImpl::TranslateBrowsePaths(
    const std::vector<scada::BrowsePath>& browse_paths,
    const scada::TranslateBrowsePathsCallback& callback) {
  auto results = sync_view_service_.TranslateBrowsePaths(browse_paths);
  callback(scada::StatusCode::Good, std::move(results));
}
