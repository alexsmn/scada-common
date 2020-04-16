#pragma once

#include "address_space/node_factory.h"

namespace scada {
class NodeId;
class TypeDefinition;
}  // namespace scada

class AddressSpaceImpl;

class GenericNodeFactory final : public NodeFactory {
 public:
  explicit GenericNodeFactory(AddressSpaceImpl& address_space)
      : address_space_{address_space} {}

  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) override;

 private:
  std::pair<scada::Status, scada::Node*> CreateNodeHelper(
      const scada::NodeState& node_state,
      const scada::NodeId& parent_id);

  void CreateProperties(scada::Node& node,
                        const scada::TypeDefinition& type_definition);

  AddressSpaceImpl& address_space_;
};
