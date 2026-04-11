#pragma once

#include "scada/attribute_service.h"
#include "scada/node_class.h"
#include "scada/node_id.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace boost::json {
class value;
}

namespace scada {

// In-memory AttributeService backed by a fixed table of nodes. Serves reads
// for `DisplayName`, `NodeClass`, and `Value` directly from the table — no
// network, no mocks. Writes are accepted and marked `Good` without side
// effects.
//
// Intended for tests, demos, and screenshot tooling that need a SCADA back-end
// driven by static data rather than a real server.
class LocalAttributeService : public AttributeService {
 public:
  struct NodeInfo {
    NodeId node_id;
    std::string display_name;
    NodeClass node_class = NodeClass::Object;
    // Mean used to synthesize Value reads when set. If unset, reads return a
    // null variant.
    std::optional<double> base_value;
  };

  LocalAttributeService();
  ~LocalAttributeService() override;

  void AddNode(NodeInfo info);

  const NodeInfo* Find(const NodeId& node_id) const;
  const std::vector<NodeInfo>& nodes() const { return nodes_; }

  // Populates the node table from the `nodes` array of a screenshot-style
  // JSON document (each entry: { id, ns, name, class, [base_value] }).
  void LoadFromJson(const boost::json::value& root);

  // AttributeService
  void Read(const ServiceContext& context,
            const std::shared_ptr<const std::vector<ReadValueId>>& inputs,
            const ReadCallback& callback) override;
  void Write(const ServiceContext& context,
             const std::shared_ptr<const std::vector<WriteValue>>& inputs,
             const WriteCallback& callback) override;

 private:
  std::vector<NodeInfo> nodes_;
  std::unordered_map<NodeId, size_t> index_;
};

}  // namespace scada
