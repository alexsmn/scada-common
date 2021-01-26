#pragma once

#include <map>

#include "address_space/node.h"

namespace scada {

class NodeBuilder;
class ObjectType;

using StatusCallback = std::function<void(Status&&)>;

class Object : public Node {
 public:
  // Node
  virtual NodeClass GetNodeClass() const override { return NodeClass::Object; }

 protected:
  Object() {}
};

class BaseObject : public Object {};

class GenericObject : public Object {
 public:
  GenericObject() {}
  GenericObject(scada::NodeId node_id,
                scada::QualifiedName browse_name,
                scada::LocalizedText display_name) {
    set_id(node_id);
    SetBrowseName(std::move(browse_name));
    SetDisplayName(std::move(display_name));
  }
};

class ComponentObject : public Object {
 public:
  ComponentObject(NodeBuilder& builder, const NodeId& instance_declaration_id);

  // Node
  virtual QualifiedName GetBrowseName() const final {
    return instance_declaration_.GetBrowseName();
  }
  virtual LocalizedText GetDisplayName() const final {
    return instance_declaration_.GetDisplayName();
  }

 private:
  Object& instance_declaration_;
};

}  // namespace scada
