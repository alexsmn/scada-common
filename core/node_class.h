#pragma once

#include <cassert>

namespace scada {

enum class NodeClass {
    Object = 1,
    Variable = 2,
    Method = 4,
    ObjectType = 8,
    VariableType = 16,
    ReferenceType = 32,
    DataType = 64,
    View = 128,
};

inline bool IsTypeDefinition(NodeClass node_class) {
  switch (node_class) {
    case NodeClass::DataType:
    case NodeClass::ObjectType:
    case NodeClass::VariableType:
    case NodeClass::ReferenceType:
      return true;
    default:
      return false;
  }
}

inline bool IsInstance(NodeClass node_class) {
  switch (node_class) {
    case NodeClass::Object:
    case NodeClass::Variable:
      return true;
    default:
      return false;
  }
}

inline const char* ToString(NodeClass node_class) {
  switch (node_class) {
    case NodeClass::Object:
      return "Object";
    case NodeClass::Variable:
      return "Variable";
    case NodeClass::Method:
      return "Method";
    case NodeClass::ObjectType:
      return "ObjectType";
    case NodeClass::VariableType:
      return "VariableType";
    case NodeClass::ReferenceType:
      return "ReferenceType";
    case NodeClass::DataType:
      return "DataType";
    case NodeClass::View:
      return "View";
    default:
      assert(false);
      return "Unknown";
  };
}

} // namespace scada
