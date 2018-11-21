#include "core/variant.h"

#include <cassert>
#include <limits>

#include "base/format.h"
#include "base/strings/utf_string_conversions.h"
#include "core/debug_util.h"
#include "core/standard_node_ids.h"

namespace scada {

namespace {

const char* kBuiltInTypeNames[] = {"EMPTY",
                                   "BOOL",
                                   "INT8",
                                   "UINT8",
                                   "INT16",
                                   "UINT16",
                                   "INT32",
                                   "UINT32",
                                   "UINT64",
                                   "INT64",
                                   "DOUBLE",
                                   "BYTE_STRING",
                                   "STRING",
                                   "QUALIFIED_NAME",
                                   "LOCALIZED_TEXT",
                                   "NODE_ID",
                                   "EXPANDED_NODE_ID",
                                   "EXTENSION_OBJECT",
                                   "DATE_TIME"};

static_assert(std::size(kBuiltInTypeNames) ==
              static_cast<size_t>(scada::Variant::COUNT));

}  // namespace

const LocalizedText Variant::kTrueString = base::WideToUTF16(L"Да");
const LocalizedText Variant::kFalseString = base::WideToUTF16(L"Нет");

void Variant::clear() {
  data_ = std::monostate{};
}

Variant& Variant::operator=(Variant&& source) {
  data_ = std::move(source.data_);
  return *this;
}

bool Variant::operator==(const Variant& other) const {
  return data_ == other.data_;
}

bool Variant::get(bool& value) const {
  if (!is_scalar())
    return false;

  if (type() == BOOL) {
    value = as_bool();
    return true;
  }

  Int64 int64_value;
  if (!get(int64_value))
    return false;

  value = int64_value != 0;
  return true;
}

bool Variant::get(Int64& value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      value = as_bool() ? 1 : 0;
      return true;
    case INT8:
      value = get<Int8>();
      return true;
    case UINT8:
      value = get<UInt8>();
      return true;
    case INT16:
      value = get<Int16>();
      return true;
    case UINT16:
      value = get<UInt16>();
      return true;
    case INT32:
      value = get<Int32>();
      return true;
    case UINT32:
      value = get<UInt32>();
      return true;
    case INT64:
      value = get<Int64>();
      return true;
    case UINT64:
      value = static_cast<Int64>(get<UInt64>());
      return true;
    case DOUBLE:
      value = static_cast<Int64>(get<Double>());
      return true;
    default:
      return false;
  }
}

bool Variant::get(Double& value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      value = as_bool() ? 1.0 : 0.0;
      return true;
    case DOUBLE:
      value = as_double();
      return true;
  }

  Int64 int64_value;
  if (!get(int64_value))
    return false;

  value = int64_value;
  return true;
}

namespace {

template <class Target, class Source>
struct FormatHelperT;

template <class Source>
struct FormatHelperT<String, Source> {
  static inline String Format(const Source& value) { return ::Format(value); }
};

template <>
struct FormatHelperT<String, QualifiedName> {
  static inline String Format(const QualifiedName& value) {
    return ToString(value);
  }
};

template <>
struct FormatHelperT<String, LocalizedText> {
  static inline String Format(const LocalizedText& value) {
    return ToString(value);
  }
};

template <class Source>
struct FormatHelperT<LocalizedText, Source> {
  static inline LocalizedText Format(const Source& value) {
    return ToLocalizedText(::Format(value));
  }
};

template <>
struct FormatHelperT<LocalizedText, QualifiedName> {
  static inline LocalizedText Format(const QualifiedName& value) {
    return ToString16(value);
  }
};

template <>
struct FormatHelperT<LocalizedText, LocalizedText> {
  static inline LocalizedText Format(const LocalizedText& value) {
    return value;
  }
};

template <class Target, class Source>
inline Target FormatHelper(const Source& value) {
  return FormatHelperT<Target, Source>::Format(value);
}

}  // namespace

template <class String>
bool Variant::ToStringHelper(String& string_value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case EMPTY:
      string_value.clear();
      return true;
    case BOOL:
      string_value =
          FormatHelper<String>(as_bool() ? kTrueString : kFalseString);
      return true;
    case DOUBLE:
      string_value = FormatHelper<String>(as_double());
      return true;
    case STRING:
      string_value = FormatHelper<String>(as_string());
      return true;
    case QUALIFIED_NAME:
      string_value = FormatHelper<String>(get<QualifiedName>());
      return true;
    case LOCALIZED_TEXT:
      string_value = FormatHelper<String>(as_localized_text());
      return true;
    case NODE_ID:
      string_value = FormatHelper<String>(as_node_id().ToString());
      return true;
    default: {
      Int64 int64_value;
      if (get(int64_value)) {
        string_value = FormatHelper<String>(int64_value);
        return true;
      }
      return false;
    }
  }
}

bool Variant::get(String& string_value) const {
  return ToStringHelper(string_value);
}

bool Variant::get(QualifiedName& value) const {
  if (!is_scalar() || type() != QUALIFIED_NAME)
    return false;
  value = get<QualifiedName>();
  return true;
}

bool Variant::get(LocalizedText& value) const {
  return ToStringHelper(value);
}

bool Variant::get(NodeId& node_id) const {
  if (!is_scalar() || type() != NODE_ID)
    return false;
  node_id = as_node_id();
  return true;
}

bool Variant::ChangeType(Variant::Type new_type) {
  if (!is_scalar())
    return false;

  if (type() == new_type)
    return true;

  switch (new_type) {
    case BOOL:
      return ChangeTypeTo<bool>();
    case INT16:
      return ChangeTypeTo<Int16>();
    case UINT16:
      return ChangeTypeTo<UInt16>();
    case INT32:
      return ChangeTypeTo<int32_t>();
    case INT64:
      return ChangeTypeTo<int64_t>();
    case DOUBLE:
      return ChangeTypeTo<double>();
    case STRING:
      return ChangeTypeTo<String>();
    case QUALIFIED_NAME:
      return ChangeTypeTo<QualifiedName>();
    case LOCALIZED_TEXT:
      return ChangeTypeTo<LocalizedText>();
    case NODE_ID:
      return ChangeTypeTo<NodeId>();
    default:
      assert(false);
      return false;
  }
}

NodeId Variant::data_type_id() const {
  static_assert(static_cast<size_t>(Type::COUNT) == 19);

  if (type() == Type::EXTENSION_OBJECT)
    return get<ExtensionObject>().data_type_id().node_id();

  const scada::NumericId kNodeIds[] = {
      0,          id::Boolean,        id::SByte,
      id::Byte,   id::Int16,          id::UInt16,
      id::Int32,  id::UInt32,         id::Int64,
      id::UInt64, id::Double,         id::ByteString,
      id::String, id::QualifiedName,  id::LocalizedText,
      id::NodeId, id::ExpandedNodeId, id::DateTime,
  };

  assert(static_cast<size_t>(type()) < std::size(kNodeIds));
  return kNodeIds[static_cast<size_t>(type())];
}

template <class T>
inline void DumpHelper(std::ostream& stream, const T& v) {
  stream << v;
}

template <>
inline void DumpHelper(std::ostream& stream, const std::monostate& v) {
  stream << "null";
}

template <>
inline void DumpHelper(std::ostream& stream, const ByteString& v) {
  stream << "\"" << FormatHexBuffer(v.data(), v.size()) << "\"";
}

template <class T>
inline void DumpHelper(std::ostream& stream, const std::vector<T>& v) {
  stream << "[";
  for (size_t i = 0; i < v.size(); ++i) {
    DumpHelper(stream, v[i]);
    if (i != v.size() - 1)
      stream << ", ";
  }
  stream << "]";
}

void Variant::Dump(std::ostream& stream) const {
  std::visit([&](const auto& v) { DumpHelper(stream, v); }, data_);
}

scada::Variant::Type ParseBuiltInType(std::string_view str) {
  auto i = std::find(std::begin(kBuiltInTypeNames), std::end(kBuiltInTypeNames),
                     str);
  return i != std::end(kBuiltInTypeNames)
             ? static_cast<scada::Variant::Type>(i -
                                                 std::begin(kBuiltInTypeNames))
             : scada::Variant::COUNT;
}

}  // namespace scada

std::string ToString(scada::Variant::Type type) {
  size_t index = static_cast<size_t>(type);
  return index < std::size(scada::kBuiltInTypeNames)
             ? scada::kBuiltInTypeNames[index]
             : "(Unknown)";
}

std::string ToString(const scada::Variant& value) {
  return value.get_or(std::string{});
}

base::string16 ToString16(const scada::Variant& value) {
  return value.get_or(base::string16{});
}
