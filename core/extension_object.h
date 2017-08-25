#pragma once

#include <memory>

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

  bool operator==(const ExtensionObject& other) const {
    return value_ == other.value_;
  }

  OpcUa_ExtensionObject release() {
    auto value = *value_;
    ::OpcUa_ExtensionObject_Initialize(value_.get());
    return value;
  }

 private:
  static void Deleter(OpcUa_ExtensionObject* object) {
    ::OpcUa_ExtensionObject_Delete(&object);
  }

  std::shared_ptr<OpcUa_ExtensionObject> value_;
};

} // namespace scada