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

NodeId::NodeId(String string_id, NamespaceIndex namespace_index)
    : namespace_index_{namespace_index},
      identifier_{std::make_shared<String>(std::move(string_id))} {
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
  return std::get<NumericId>(identifier_);
}

const String* NodeId::string_id() const {
  auto* string_id = std::get_if<SharedStringId>(&identifier_);
  return string_id ? string_id->get() : nullptr;
}

const ByteString* NodeId::opaque_id() const {
  auto* opaque_id = std::get_if<SharedByteString>(&identifier_);
  return opaque_id ? opaque_id->get() : nullptr;
}

String NodeId::ToString() const {
  std::string result;

  if (namespace_index_ != 0)
    result += base::StringPrintf("ns=%u;", static_cast<unsigned>(namespace_index_));

  switch (type()) {
    case NodeIdType::Numeric:
      result += base::StringPrintf("i=%u", static_cast<unsigned>(numeric_id()));
      break;
    case NodeIdType::String:
      result += base::StringPrintf("s=%s", string_id()->c_str());
      break;
    case NodeIdType::Opaque:
      // TODO:
    default:
      assert(false);
  }

  return result;
}

// static
NodeId NodeId::FromString(const base::StringPiece& string) {
  if (string.empty())
    return {};

  NamespaceIndex namespace_index = 0;

  base::StringPiece str = string;

  if (str.starts_with("ns=")) {
    auto index = str.find(';');
    if (index == base::StringPiece::npos)
      return {};
    unsigned id = 0;
    if (!base::StringToUint(str.substr(3, index - 3), &id))
      return {};
    namespace_index = static_cast<NamespaceIndex>(id);
    str = str.substr(index + 1);
  }

  if (str.starts_with("i=")) {
    unsigned numeric_id = 0;
    if (!base::StringToUint(str.substr(2), &numeric_id))
      return {};
    return {numeric_id, namespace_index};
  }

  if (str.starts_with("s=")) {
    auto string_id = str.substr(2);
    return {string_id.as_string(), namespace_index};
  }

  if (str.starts_with("s=")) {
    auto string_id = str.substr(2);
    return {string_id.as_string(), namespace_index};
  }

  // TODO: g=
  // TODO: b=

  return {str.as_string(), namespace_index};
}

} // namespace scada