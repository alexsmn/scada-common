#pragma once

#include "address_space/node_factory.h"

namespace scada {
class NodeId;
}

class AddressSpaceImpl;
class Logger;

class GenericNodeFactory final : public NodeFactory {
 public:
  GenericNodeFactory(std::shared_ptr<Logger> logger,
                     AddressSpaceImpl& address_space)
      : logger_{std::move(logger)}, address_space_{address_space} {}

  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) override;

 private:
  std::pair<scada::Status, scada::Node*> CreateNodeHelper(
      const scada::NodeState& node_state,
      const scada::NodeId& parent_id);

  const std::shared_ptr<Logger> logger_;
  AddressSpaceImpl& address_space_;
};
