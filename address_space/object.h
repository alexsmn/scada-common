#pragma once

#include <map>

#include "address_space/node.h"

namespace scada {

class AddressSpace;
class ObjectType;

class Object : public Node {
 public:
  // Node
  virtual NodeClass GetNodeClass() const override { return NodeClass::Object; }

 protected:
  Object() {}
};

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

  // Node
  virtual Variant GetPropertyValue(const NodeId& prop_decl_id) const override;
  virtual Status SetPropertyValue(const NodeId& prop_decl_id,
                                  const Variant& value) override;

 private:
  std::map<NodeId, Variant> properties_;
};

class ComponentObject : public GenericObject {
 public:
  bool InitNode(AddressSpace& address_space,
                const NodeId& instance_declaration_id);

  // Node
  virtual QualifiedName GetBrowseName() const final {
    return instance_declaration_->GetBrowseName();
  }
  virtual LocalizedText GetDisplayName() const final {
    return instance_declaration_->GetDisplayName();
  }

 private:
  Object* instance_declaration_ = nullptr;
};

}  // namespace scada
