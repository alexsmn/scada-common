#pragma once

#include "address_space/address_space.h"
#include "address_space/node_builder.h"

class NodeBuilderImpl : public scada::NodeBuilder {
 public:
  explicit NodeBuilderImpl(scada::AddressSpace& address_space)
      : address_space_{address_space} {}

  virtual scada::Node& GetNode(const scada::NodeId& node_id) override {
    auto* node = address_space_.GetNode(node_id);
    assert(node);
    return *node;
  }

  virtual void AddReference(const scada::NodeId& reference_type_id,
                            scada::Node& source,
                            scada::Node& target) override {
    scada::AddReference(address_space_, reference_type_id, source, target);
  }

 private:
  scada::AddressSpace& address_space_;
};
