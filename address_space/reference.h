#pragma once

namespace scada {

class Node;
class ReferenceType;

struct Reference {
  const ReferenceType* type;
  Node* node;

  explicit operator bool() const { return node != nullptr; }

  bool operator==(const Reference& other) const {
    return type == other.type && node == other.node;
  }
};

} // namespace scada
