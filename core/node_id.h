#pragma once

#include "base/strings/string_piece.h"

#include <boost/variant.hpp>
#include <cassert>
#include <opcua_platformdefs.h>
#include <opcua_builtintypes.h>
#include <string>
#include <vector>

namespace scada {

enum class NodeIdType { Numeric, String, Opaque };

using NamespaceIndex = OpcUa_UInt16;
using NumericId = OpcUa_UInt32;
using StringId = std::string;
using ByteString = std::vector<char>;

class NodeId {
 public:
  NodeId();
  NodeId(NumericId numeric_id, NamespaceIndex namespace_index = 0);
  NodeId(StringId string_id, NamespaceIndex namespace_index);
  NodeId(ByteString opaque_id, NamespaceIndex namespace_index);

  NodeIdType type() const { return static_cast<NodeIdType>(identifier_.which()); }

  bool is_null() const;

  NamespaceIndex namespace_index() const { return namespace_index_; }
  void set_namespace_index(NamespaceIndex index) { namespace_index_ = index; }

  NumericId numeric_id() const;
  const std::string& string_id() const;
  const ByteString& opaque_id() const;

  std::string ToString() const;
  static NodeId FromString(const base::StringPiece& string);

 private:
  using SharedStringId = std::shared_ptr<const StringId>;
  using SharedByteString = std::shared_ptr<const ByteString>;
  boost::variant<NumericId, SharedStringId, SharedByteString> identifier_;

  NamespaceIndex namespace_index_;
};

bool operator==(const NodeId& a, const NodeId& b);
inline bool operator!=(const NodeId& a, const NodeId& b) { return !operator==(a, b); }

bool operator<(const NodeId& a, const NodeId& b);

} // namespace scada