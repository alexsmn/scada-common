#include "address_space/local_attribute_service.h"

#include "base/time/time.h"
#include "scada/data_value.h"
#include "scada/service_context.h"
#include "scada/status.h"

#include <boost/json.hpp>

#include <random>

namespace scada {

namespace {

DataValue MakeValue(Variant value) {
  const auto timestamp = base::Time::Now();
  return DataValue{std::move(value), {}, timestamp, timestamp};
}

// Deterministic noise around `base_value` — matches the behavior the
// screenshot generator previously obtained from its ON_CALL Read lambda.
double NoisyValue(double base_value) {
  static std::mt19937 rng{42};
  std::normal_distribution<double> dist(base_value,
                                        std::abs(base_value) * 0.01);
  return dist(rng);
}

}  // namespace

LocalAttributeService::LocalAttributeService() = default;
LocalAttributeService::~LocalAttributeService() = default;

void LocalAttributeService::AddNode(NodeInfo info) {
  auto id = info.node_id;
  nodes_.push_back(std::move(info));
  index_[id] = nodes_.size() - 1;
}

const LocalAttributeService::NodeInfo* LocalAttributeService::Find(
    const NodeId& node_id) const {
  auto it = index_.find(node_id);
  return it != index_.end() ? &nodes_[it->second] : nullptr;
}

void LocalAttributeService::LoadFromJson(const boost::json::value& root) {
  for (const auto& jn : root.at("nodes").as_array()) {
    NodeInfo info;
    auto id = static_cast<NumericId>(jn.at("id").as_int64());
    auto ns = static_cast<NamespaceIndex>(jn.at("ns").as_int64());
    info.node_id = NodeId{id, ns};
    info.display_name = std::string(jn.at("name").as_string());
    auto cls = jn.at("class").as_string();
    info.node_class =
        (cls == "variable") ? NodeClass::Variable : NodeClass::Object;
    if (auto* bv = jn.as_object().if_contains("base_value"))
      info.base_value = bv->to_number<double>();
    AddNode(std::move(info));
  }
}

void LocalAttributeService::Read(
    const ServiceContext& /*context*/,
    const std::shared_ptr<const std::vector<ReadValueId>>& inputs,
    const ReadCallback& callback) {
  std::vector<DataValue> results;
  results.reserve(inputs->size());

  for (const auto& input : *inputs) {
    const auto* info = Find(input.node_id);
    switch (input.attribute_id) {
      case AttributeId::DisplayName:
        results.push_back(MakeValue(Variant{
            info ? info->display_name
                 : "Node " + std::to_string(input.node_id.is_numeric()
                                                ? input.node_id.numeric_id()
                                                : 0)}));
        break;

      case AttributeId::Value:
        if (info && info->base_value) {
          results.push_back(MakeValue(Variant{NoisyValue(*info->base_value)}));
        } else {
          results.push_back(MakeValue(Variant{0.0}));
        }
        break;

      case AttributeId::NodeClass:
        results.push_back(MakeValue(
            Variant{static_cast<Int32>(info ? info->node_class
                                            : NodeClass::Object)}));
        break;

      default:
        results.push_back(MakeValue(Variant{}));
        break;
    }
  }

  callback(Status{StatusCode::Good}, std::move(results));
}

void LocalAttributeService::Write(
    const ServiceContext& /*context*/,
    const std::shared_ptr<const std::vector<WriteValue>>& inputs,
    const WriteCallback& callback) {
  std::vector<StatusCode> results(inputs->size(), StatusCode::Good);
  callback(Status{StatusCode::Good}, std::move(results));
}

}  // namespace scada
