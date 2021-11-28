#pragma once

#include "address_space/node.h"

namespace scada {

class Method : public Node {
 public:
  // Node
  virtual NodeClass GetNodeClass() const override final {
    return NodeClass::Method;
  }

 protected:
  Method() = default;
};

class GenericMethod : public Method {
 public:
  GenericMethod(scada::NodeId node_id,
                scada::QualifiedName browse_name,
                scada::LocalizedText display_name) {
    set_id(std::move(node_id));
    SetBrowseName(std::move(browse_name));
    SetDisplayName(std::move(display_name));
  }
};

}  // namespace scada
