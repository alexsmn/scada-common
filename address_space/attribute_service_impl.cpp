#include "attribute_service_impl.h"

#include "address_space/address_space.h"
#include "address_space/address_space_util.h"
#include "address_space/node.h"
#include "address_space/node_utils.h"
#include "address_space/type_definition.h"
#include "address_space/variable.h"

#include <atomic>
#include <unordered_map>

// TODO: Remove
#include "model/scada_node_ids.h"

AttributeServiceImpl::AttributeServiceImpl(
    AttributeServiceImplContext&& context)
    : AttributeServiceImplContext{std::move(context)} {}

void AttributeServiceImpl::Read(
    const std::vector<scada::ReadValueId>& descriptions,
    const scada::ReadCallback& callback) {
  std::vector<scada::DataValue> results(descriptions.size());

  struct AsyncRequest {
    std::vector<scada::ReadValueId> descriptions;
    std::vector<size_t> result_indexes;
  };

  std::unordered_map<scada::AttributeService*, AsyncRequest> async_requests;

  for (size_t index = 0; index < descriptions.size(); ++index) {
    scada::AttributeService* async_service = nullptr;
    results[index] = Read(descriptions[index], async_service);
    if (async_service) {
      auto& async_request = async_requests[async_service];
      async_request.descriptions.emplace_back(descriptions[index]);
      async_request.result_indexes.emplace_back(index);
    }
  }

  if (async_requests.empty())
    return callback(scada::StatusCode::Good, std::move(results));

  struct SharedResults {
    std::vector<scada::DataValue> results;
    scada::ReadCallback callback;
    std::atomic<int> count;
  };

  auto shared_data = std::make_shared<SharedResults>();
  shared_data->results = std::move(results);
  shared_data->callback = std::move(callback);
  shared_data->count = static_cast<int>(async_requests.size());

  for (auto& p : async_requests) {
    auto& service = *p.first;
    auto& async_request = p.second;

    service.Read(
        async_request.descriptions,
        [shared_data, result_indexes = std::move(async_request.result_indexes)](
            scada::Status&& status, std::vector<scada::DataValue>&& results) {
          assert(!status || result_indexes.size() == results.size());
          for (size_t i = 0; i < result_indexes.size(); ++i) {
            auto& result = shared_data->results[result_indexes[i]];
            if (status)
              result = std::move(results[i]);
            else
              result = {status.code(), scada::DateTime::Now()};
          }

          auto remaining_count = --shared_data->count;
          assert(remaining_count >= 0);
          if (remaining_count == 0)
            shared_data->callback(scada::StatusCode::Good,
                                  std::move(shared_data->results));
        });
  }
}

void AttributeServiceImpl::Write(const scada::WriteValue& value,
                                 const scada::NodeId& user_id,
                                 const scada::StatusCallback& callback) {
  callback(scada::StatusCode::Bad_WrongNodeId);
}

scada::DataValue AttributeServiceImpl::Read(
    const scada::ReadValueId& read_id,
    scada::AttributeService*& async_view_service) {
  async_view_service = nullptr;

  base::StringPiece nested_name;
  auto* node =
      scada::GetNestedNode(address_space_, read_id.node_id, nested_name);
  if (!node)
    return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};

  if (nested_name.empty())
    return ReadNode(*node, read_id.attribute_id);

  return {scada::StatusCode::Bad_WrongNodeId, scada::DateTime::Now()};
}

scada::DataValue AttributeServiceImpl::ReadNode(
    const scada::Node& node,
    scada::AttributeId attribute_id) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass:
      return scada::MakeReadResult(static_cast<int>(node.GetNodeClass()));

    case scada::AttributeId::BrowseName:
      assert(!node.GetBrowseName().empty());
      return scada::MakeReadResult(node.GetBrowseName());

    case scada::AttributeId::DisplayName:
      return scada::MakeReadResult(node.GetDisplayName());
  }

  if (auto* variable = scada::AsVariable(&node)) {
    switch (attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(variable->GetDataType().id());
      case scada::AttributeId::Value:
        return variable->GetValue();
    }
  }

  if (auto* variable_type = scada::AsVariableType(&node)) {
    switch (attribute_id) {
      case scada::AttributeId::DataType:
        return scada::MakeReadResult(variable_type->data_type().id());
      case scada::AttributeId::Value:
        return scada::MakeReadResult(variable_type->default_value());
    }
  }

  return {scada::StatusCode::Bad_WrongAttributeId, scada::DateTime::Now()};
}
