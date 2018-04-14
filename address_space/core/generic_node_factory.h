#pragma once

#include "core/node_factory.h"

class ConfigurationImpl;
class Logger;

class GenericNodeFactory final : public NodeFactory {
 public:
  GenericNodeFactory(std::shared_ptr<Logger> logger,
                     ConfigurationImpl& address_space)
      : logger_{std::move(logger)}, address_space_{address_space} {}

  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) override;

 private:
  const std::shared_ptr<Logger> logger_;
  ConfigurationImpl& address_space_;
};
