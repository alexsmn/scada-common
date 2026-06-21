#include "view_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "base/range_util.h"
#include "base/span_util.h"
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

  auto* current_node = TranslateBrowsePathHelper2(node, relative_path[0]);
  if (!current_node)
    return {scada::StatusCode::Bad_BrowseNameInvalid};

  size_t i = 1;
  for (; i < relative_path.size(); ++i) {
    auto* next_node =
        TranslateBrowsePathHelper2(*current_node, relative_path[i]);
    if (!next_node)
      break;
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
    std::span<const scada::BrowseDescription> inputs) {
  return AsRange(inputs) |
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
    std::span<const scada::BrowsePath> inputs) {
  return AsRange(inputs) |
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

  // OPC UA Part 4 §7.3: nodeClassMask filters returned references by the target
  // node's NodeClass (0 = any).
  // https://reference.opcfoundation.org/Core/Part4/v105/docs/7.3
  const auto matches_node_class = [&](scada::NodeClass node_class) {
    return description.node_class_mask == 0 ||
           (description.node_class_mask &
            static_cast<scada::UInt32>(node_class)) != 0;
  };

  // OPC UA Part 4 §7.3: resultMask selects which ReferenceDescription fields to
  // populate; unrequested fields stay at their defaults.
  const auto mask = description.result_mask;
  const auto make_reference = [&](const auto& ref, bool forward) {
    scada::ReferenceDescription desc;
    if (mask & scada::kBrowseResultReferenceType)
      desc.reference_type_id = ref.type->id();
    desc.forward = (mask & scada::kBrowseResultIsForward) ? forward : false;
    desc.node_id = ref.node->id();
    if (mask & scada::kBrowseResultNodeClass)
      desc.node_class = ref.node->GetNodeClass();
    if (mask & scada::kBrowseResultBrowseName)
      desc.browse_name = ref.node->GetBrowseName();
    if (mask & scada::kBrowseResultDisplayName)
      desc.display_name = ref.node->GetDisplayName();
    if ((mask & scada::kBrowseResultTypeDefinition) &&
        ref.node->type_definition()) {
      desc.type_definition = ref.node->type_definition()->id();
    }
    return desc;
  };

  if (description.direction == scada::BrowseDirection::Forward ||
      description.direction == scada::BrowseDirection::Both) {
    for (const auto& ref : scada::FilterReferences(
             node.forward_references(), description.reference_type_id,
             description.include_subtypes)) {
      // Skip references to targets not present in the address space rather than
      // dereferencing a null node.
      if (!ref.node)
        continue;
      if (!matches_node_class(ref.node->GetNodeClass()))
        continue;
      result.references.push_back(make_reference(ref, true));
    }
  }

  if (description.direction == scada::BrowseDirection::Inverse ||
      description.direction == scada::BrowseDirection::Both) {
    for (const auto& ref : scada::FilterReferences(
             node.inverse_references(), description.reference_type_id,
             description.include_subtypes)) {
      if (!ref.node)
        continue;
      if (!matches_node_class(ref.node->GetNodeClass()))
        continue;
      result.references.push_back(make_reference(ref, false));
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
      result.references.emplace_back(scada::id::HasTypeDefinition,
                                     /*forward=*/true, scada::id::PropertyType,
                                     scada::NodeClass::VariableType);
  }

  if (description.direction == scada::BrowseDirection::Inverse ||
      description.direction == scada::BrowseDirection::Both) {
    if (scada::IsSubtypeOf(address_space_, scada::id::HasProperty,
                           description.reference_type_id))
      result.references.emplace_back(scada::id::HasProperty, /*forward=*/false,
                                     node.id(), node.GetNodeClass());
  }

  return result;
}

Awaitable<scada::StatusOr<std::vector<scada::BrowseResult>>>
ViewServiceImpl::Browse(
    scada::ServiceContext context,
    std::vector<scada::BrowseDescription> descriptions) {
  auto results = sync_view_service_.Browse(descriptions);
  co_return results;
}

Awaitable<scada::StatusOr<std::vector<scada::BrowsePathResult>>>
ViewServiceImpl::TranslateBrowsePaths(
    std::vector<scada::BrowsePath> browse_paths) {
  auto results = sync_view_service_.TranslateBrowsePaths(browse_paths);
  co_return results;
}
