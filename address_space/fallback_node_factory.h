#pragma once

#include "address_space/node_factory.h"
#include "base/containers/span.h"

class FallbackNodeFactory : public NodeFactory {
 public:
  explicit FallbackNodeFactory(base::span<NodeFactory*> node_factories)
      : node_factories_{node_factories} {}

  // NodeFactory
  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) override;

  const base::span<NodeFactory*> node_factories_;
};

inline std::pair<scada::Status, scada::Node*> FallbackNodeFactory::CreateNode(
    const scada::NodeState& node_state) {
  for (auto* node_factory : node_factories_) {
    auto result = node_factory->CreateNode(node_state);
    if (result.first)
      return result;
  }

  return {scada::StatusCode::Bad, nullptr};
}
