#pragma once

#include <cassert>
#include <string>

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

} // namespace scada
inline std::string ToString(scada::NodeClass node_class) {
  switch (node_class) {
    case scada::NodeClass::Object:
      return "Object";
    case scada::NodeClass::Variable:
      return "Variable";
    case scada::NodeClass::Method:
      return "Method";
    case scada::NodeClass::ObjectType:
      return "ObjectType";
    case scada::NodeClass::VariableType:
      return "VariableType";
    case scada::NodeClass::ReferenceType:
      return "ReferenceType";
    case scada::NodeClass::DataType:
      return "DataType";
    case scada::NodeClass::View:
      return "View";
    default:
      assert(false);
      return "Unknown";
  };
}

