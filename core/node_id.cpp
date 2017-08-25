#include "core/node_id.h"

#include <atomic>

#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "core/namespaces.h"

namespace scada {

NodeId::NodeId()
    : namespace_index_{0},
      identifier_{0} {
}

NodeId::NodeId(NumericId numeric_id, NamespaceIndex namespace_index)
    : namespace_index_{namespace_index},
      identifier_{numeric_id} {
}

NodeId::NodeId(std::string string_id, NamespaceIndex namespace_index)
    : namespace_index_{namespace_index},
      identifier_{std::make_shared<StringId>(std::move(string_id))} {
}

NodeId::NodeId(ByteString opaque_id, NamespaceIndex namespace_index)
    : namespace_index_{namespace_index},
      identifier_{std::make_shared<ByteString>(std::move(opaque_id))} {
}

bool NodeId::is_null() const {
  return namespace_index_ == 0 && type() == NodeIdType::Numeric && numeric_id() == 0;
}

bool operator==(const NodeId& a, const NodeId& b) {
  if (a.namespace_index() != b.namespace_index())
    return false;

  if (a.type() != b.type())
    return false;

  switch (a.type()) {
    case NodeIdType::Numeric:
      return a.numeric_id() == b.numeric_id();
    case NodeIdType::String:
      return a.string_id() == b.string_id();
    case NodeIdType::Opaque:
      return a.opaque_id() == b.opaque_id();
    default:
      assert(false);
      return false;
  }
}

bool operator<(const NodeId& a, const NodeId& b) {
  if (a.namespace_index() != b.namespace_index())
    return a.namespace_index() < b.namespace_index();

  if (a.type() != b.type())
    return a.type() < b.type();

  switch (a.type()) {
    case NodeIdType::Numeric:
      return a.numeric_id() < b.numeric_id();
    case NodeIdType::String:
      return a.string_id() < b.string_id();
    case NodeIdType::Opaque:
      return a.opaque_id() < b.opaque_id();
    default:
      assert(false);
      return false;
  }
}

NumericId NodeId::numeric_id() const {
  return boost::get<NumericId>(identifier_);
}

const std::string& NodeId::string_id() const {
  return *boost::get<SharedStringId>(identifier_);
}

const ByteString& NodeId::opaque_id() const {
  return *boost::get<SharedByteString>(identifier_);
}

std::string NodeId::ToString() const {
  std::string result;

  switch (type()) {
    case NodeIdType::Numeric:
      result = base::StringPrintf("%s|%u", GetNamespaceName(namespace_index()), numeric_id());
      break;
    case NodeIdType::String:
      result = base::StringPrintf("%u|%s", namespace_index(), string_id().c_str());
      break;
    case NodeIdType::Opaque:
      result = base::StringPrintf("%u|%s", namespace_index(), base::HexEncode(opaque_id().data(), opaque_id().size()).c_str());
      break;
    default:
      assert(false);
  }

  return result;
}

// static
NodeId NodeId::FromString(const base::StringPiece& string) {
  NamespaceIndex namespace_index = 0;
  auto identifier = string;
  auto p = string.find('|');
  if (p != base::StringPiece::npos) {
    auto namespace_name = string.substr(0, p);
    auto ni = FindNamespaceIndexByName(namespace_name);
    if (ni == -1)
      return {};
    namespace_index = static_cast<NamespaceIndex>(ni);
    identifier = string.substr(p + 1);
  }

  int numeric_id = 0;
  if (base::StringToInt(identifier, &numeric_id))
    return NodeId{static_cast<NumericId>(numeric_id), namespace_index};

  return NodeId{identifier.as_string(), namespace_index};
}

} // namespace scada