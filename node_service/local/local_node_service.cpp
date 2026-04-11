#include "node_service/local/local_node_service.h"

#include <boost/json.hpp>

#include <string>

LocalNodeService::LocalNodeService() = default;
LocalNodeService::~LocalNodeService() = default;

NodeRef LocalNodeService::AddNode(LocalNodeModel::Data data) {
  auto id = data.node_id;
  auto model = std::make_shared<LocalNodeModel>(std::move(data));
  NodeRef ref{model};
  refs_[id] = ref;
  return ref;
}

void LocalNodeService::LoadFromJson(const boost::json::value& root) {
  for (const auto& jn : root.at("nodes").as_array()) {
    LocalNodeModel::Data data;
    auto id = static_cast<scada::NumericId>(jn.at("id").as_int64());
    auto ns = static_cast<scada::NamespaceIndex>(jn.at("ns").as_int64());
    data.node_id = scada::NodeId{id, ns};
    data.display_name = std::string(jn.at("name").as_string());
    auto cls = jn.at("class").as_string();
    data.node_class = (cls == "variable") ? scada::NodeClass::Variable
                                          : scada::NodeClass::Object;
    AddNode(std::move(data));
  }
}

NodeRef LocalNodeService::GetNode(const scada::NodeId& node_id) {
  auto it = refs_.find(node_id);
  return it != refs_.end() ? it->second : NodeRef{};
}
