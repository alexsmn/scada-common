#pragma once

#include "address_space/node_factory.h"

namespace scada {
class NodeId;
}  // namespace scada

class MutableAddressSpace;

class GenericNodeFactory final : public NodeFactory {
 public:
  explicit GenericNodeFactory(MutableAddressSpace& address_space,
                              bool create_properties = true)
      : address_space_{address_space}, create_properties_{create_properties} {}

  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) override;

 private:
  std::pair<scada::Status, scada::Node*> CreateNodeHelper(
      const scada::NodeState& node_state,
      const scada::NodeId& parent_id);

  MutableAddressSpace& address_space_;
  const bool create_properties_ = false;
};
