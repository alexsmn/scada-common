#include "view_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"

#include <atomic>
#include <unordered_map>

ViewServiceImpl::ViewServiceImpl(ViewServiceImplContext&& context)
    : ViewServiceImplContext{std::move(context)} {}

ViewServiceImpl::~ViewServiceImpl() {}

scada::BrowseResult ViewServiceImpl::Browse(
    const scada::BrowseDescription& description,
    scada::ViewService*& async_view_service) {
  async_view_service = nullptr;

  base::StringPiece nested_name;
  auto* node = GetNestedNode(address_space_, description.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId};

  if (nested_name.empty())
    return BrowseNode(*node, description);

  return BrowseProperty(*node, nested_name, description);
}

scada::BrowseResult ViewServiceImpl::BrowseNode(
    const scada::Node& node,
    const scada::BrowseDescription& description) {
  scada::BrowseResult result;
  result.status_code = scada::StatusCode::Good;

  if (!description.include_subtypes) {
    assert(false);
    result.status_code = scada::StatusCode::Bad;
    return result;
  }

  if (description.direction == scada::BrowseDirection::Forward ||
      description.direction == scada::BrowseDirection::Both) {
    for (const auto& ref : scada::FilterReferences(
             node.forward_references(), description.reference_type_id)) {
      assert(!ref.type->id().is_null());
      assert(!ref.node->id().is_null());
      result.references.push_back({ref.type->id(), true, ref.node->id()});
    }
  }

  if (description.direction == scada::BrowseDirection::Inverse ||
      description.direction == scada::BrowseDirection::Both) {
    for (const auto& ref : scada::FilterReferences(
             node.inverse_references(), description.reference_type_id)) {
      assert(!ref.type->id().is_null());
      assert(!ref.node->id().is_null());
      result.references.push_back({ref.type->id(), false, ref.node->id()});
    }
  }

  return result;
}

scada::BrowseResult ViewServiceImpl::BrowseProperty(
    const scada::Node& node,
    base::StringPiece nested_name,
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

  struct AsyncBrowseData {
    std::vector<scada::BrowseDescription> descriptions;
    std::vector<size_t> result_indexes;
  };

  std::unordered_map<scada::ViewService*, AsyncBrowseData> async_nodes;

  for (size_t index = 0; index < descriptions.size(); ++index) {
    scada::ViewService* async_view_service = nullptr;
    results[index] = Browse(descriptions[index], async_view_service);
    if (async_view_service) {
      auto& browse_data = async_nodes[async_view_service];
      browse_data.descriptions.emplace_back(descriptions[index]);
      browse_data.result_indexes.emplace_back(index);
    }
  }

  if (async_nodes.empty())
    return callback(scada::StatusCode::Good, std::move(results));

  struct SharedResults {
    std::vector<scada::BrowseResult> results;
    scada::BrowseCallback callback;
    std::atomic<int> count;
  };

  auto shared_data = std::make_shared<SharedResults>();
  shared_data->results = std::move(results);
  shared_data->callback = std::move(callback);
  shared_data->count = static_cast<int>(async_nodes.size());

  for (auto& p : async_nodes) {
    auto& view_service = *p.first;
    auto& browse_data = p.second;

    view_service.Browse(
        browse_data.descriptions,
        [shared_data, result_indexes = std::move(browse_data.result_indexes)](
            scada::Status&& status,
            std::vector<scada::BrowseResult>&& results) {
          if (status) {
            assert(result_indexes.size() == results.size());
            for (size_t i = 0; i < result_indexes.size(); ++i) {
              auto& result = shared_data->results[result_indexes[i]];
              auto& references = results[i].references;
              if (scada::Status{results[i].status_code}) {
                result.references.insert(
                    result.references.end(),
                    std::make_move_iterator(references.begin()),
                    std::make_move_iterator(references.end()));
                result.status_code = results[i].status_code;
              }
            }
          }

          auto remaining_count = --shared_data->count;
          assert(remaining_count >= 0);
          if (remaining_count == 0)
            shared_data->callback(scada::StatusCode::Good,
                                  std::move(shared_data->results));
        });
  }
}

void ViewServiceImpl::TranslateBrowsePath(
    const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  assert(false);
  callback(scada::StatusCode::Bad, {}, 0);
}
