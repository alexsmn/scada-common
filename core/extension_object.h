#pragma once

#include <memory>

#include "core/node_id.h"
#include "opcuapp/extension_object.h"

namespace scada {

class ExtensionObject {
 public:
  ExtensionObject() {}

  ExtensionObject(OpcUa_ExtensionObject&& source) {
    OpcUa_ExtensionObject* object = OpcUa_Null;
    OpcUa_ExtensionObject_Create(&object);
    *object = source;
    OpcUa_ExtensionObject_Initialize(&source);
    value_ = std::shared_ptr<OpcUa_ExtensionObject>(object, &Deleter);
  }

  NodeId data_type_id() const {
    if (!value_)
      return {};
    if (value_->TypeId.ServerIndex != 0 || !OpcUa_String_IsEmpty(&value_->TypeId.NamespaceUri))
      return {};
    if (value_->TypeId.NodeId.IdentifierType != OpcUa_IdentifierType_Numeric)
      return {};
    return {value_->TypeId.NodeId.Identifier.Numeric, value_->TypeId.NodeId.NamespaceIndex};
  }

  bool operator==(const ExtensionObject& other) const {
    return value_ == other.value_;
  }

  void release(OpcUa_ExtensionObject& target) {
    target = *value_;
    ::OpcUa_ExtensionObject_Initialize(value_.get());
  }

 private:
  static void Deleter(OpcUa_ExtensionObject* object) {
    ::OpcUa_ExtensionObject_Delete(&object);
  }

  std::shared_ptr<OpcUa_ExtensionObject> value_;
};

} // namespace scada