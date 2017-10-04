#pragma once

#include "core/node_id.h"

#include <memory>
#include <opcuapp/extension_object.h>

namespace scada {

class ExtensionObject {
 public:
  ExtensionObject() {}

  ExtensionObject(OpcUa_ExtensionObject&& source)
      : value_{std::make_shared<opcua::ExtensionObject>(std::move(source))} {
  }

  ExtensionObject(opcua::ExtensionObject&& source)
      : value_{std::make_shared<opcua::ExtensionObject>(std::move(source))} {
  }

  NodeId data_type_id() const {
    if (!value_)
      return {};
    if (value_->type_id().ServerIndex != 0 || !OpcUa_String_IsEmpty(&value_->type_id().NamespaceUri))
      return {};
    if (value_->type_id().NodeId.IdentifierType != OpcUa_IdentifierType_Numeric)
      return {};
    return {value_->type_id().NodeId.Identifier.Numeric, value_->type_id().NodeId.NamespaceIndex};
  }

  bool operator==(const ExtensionObject& other) const {
    return value_ == other.value_;
  }

  void release(OpcUa_ExtensionObject& target) {
    if (value_)
      target = value_->get();
    else
      opcua::Initialize(target);
    value_.reset();
  }

 private:
  std::shared_ptr<opcua::ExtensionObject> value_;
};

} // namespace scada