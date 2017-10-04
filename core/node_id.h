#pragma once

#include "base/strings/string_piece.h"
#include "core/basic_types.h"
#include "core/string.h"

#include <cassert>
#include <string>
#include <variant>
#include <vector>

namespace scada {

enum class NodeIdType { Numeric, String, Opaque };

using NumericId = uint32_t;

class NodeId {
 public:
  NodeId();
  NodeId(NumericId numeric_id, NamespaceIndex namespace_index = 0);
  NodeId(String string_id, NamespaceIndex namespace_index);
  NodeId(ByteString opaque_id, NamespaceIndex namespace_index);

  NodeIdType type() const { return static_cast<NodeIdType>(identifier_.index()); }

  bool is_null() const;

  NamespaceIndex namespace_index() const { return namespace_index_; }
  void set_namespace_index(NamespaceIndex index) { namespace_index_ = index; }

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
inline bool operator!=(const NodeId& a, const NodeId& b) { return !operator==(a, b); }

bool operator<(const NodeId& a, const NodeId& b);

} // namespace scada