#pragma once

#include "base/strings/string_piece.h"
#include "core/basic_types.h"
#include "core/string.h"

#include <cassert>
#include <memory>
#include <variant>

namespace scada {

enum class NodeIdType { Numeric, String, Opaque };

using NumericId = uint32_t;

class NodeId {
 public:
  NodeId() noexcept;
  NodeId(NumericId numeric_id, NamespaceIndex namespace_index = 0) noexcept;
  NodeId(String string_id, NamespaceIndex namespace_index) noexcept;
  NodeId(ByteString opaque_id, NamespaceIndex namespace_index) noexcept;

  NodeIdType type() const noexcept {
    return static_cast<NodeIdType>(identifier_.index());
  }

  bool is_null() const noexcept;

  NamespaceIndex namespace_index() const noexcept { return namespace_index_; }
  void set_namespace_index(NamespaceIndex index) noexcept {
    namespace_index_ = index;
  }

  NumericId numeric_id() const;
  const String& string_id() const;
  const ByteString& opaque_id() const;

  String ToString() const;
  static NodeId FromString(const base::StringPiece& string);

 private:
  using SharedStringId = std::shared_ptr<const String>;
  using SharedByteString = std::shared_ptr<const ByteString>;
  std::variant<NumericId, SharedStringId, SharedByteString> identifier_;

  NamespaceIndex namespace_index_;
};

bool operator==(const NodeId& a, const NodeId& b);

inline bool operator!=(const NodeId& a, const NodeId& b) {
  return !operator==(a, b);
}

bool operator<(const NodeId& a, const NodeId& b);

bool operator==(const NodeId& a, NumericId b);

inline bool operator!=(const NodeId& a, NumericId b) {
  return !(a == b);
}

inline bool operator==(NumericId a, const NodeId& b) {
  return b == a;
}

inline bool operator!=(NumericId a, const NodeId& b) {
  return !operator==(a, b);
}

inline std::ostream& operator<<(std::ostream& stream, const NodeId& node_id) {
  return stream << "\"" << node_id.ToString() << "\"";
}

}  // namespace scada
