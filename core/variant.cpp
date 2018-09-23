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
                                   "INT32",
                                   "UINT32",
                                   "INT64",
                                   "DOUBLE",
                                   "BYTE_STRING",
                                   "STRING",
                                   "QUALIFIED_NAME",
                                   "LOCALIZED_TEXT",
                                   "NODE_ID",
                                   "EXPANDED_NODE_ID",
                                   "EXTENSION_OBJECT"};

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

bool Variant::get_or(bool or_value) const {
  bool bool_value;
  return get(bool_value) ? bool_value : or_value;
}

int32_t Variant::get_or(int32_t or_value) const {
  int32_t int_value;
  return get(int_value) ? int_value : or_value;
}

double Variant::get_or(double or_value) const {
  double double_value;
  return get(double_value) ? double_value : or_value;
}

std::string Variant::get_or(std::string&& or_value) const {
  std::string string_value = std::move(or_value);
  get(string_value);
  return string_value;
}

QualifiedName Variant::get_or(QualifiedName&& or_value) const {
  QualifiedName value = std::move(or_value);
  get(value);
  return value;
}

LocalizedText Variant::get_or(LocalizedText&& or_value) const {
  LocalizedText value = std::move(or_value);
  get(value);
  return value;
}

bool Variant::get(bool& bool_value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      bool_value = as_bool();
      return true;
    case INT32:
      bool_value = as_int32() != 0;
      return true;
    case INT64:
      bool_value = as_int64() != 0;
      return true;
    case DOUBLE:
      bool_value = abs(as_double()) >= std::numeric_limits<double>::epsilon();
      return true;
    default:
      return false;
  }
}

bool Variant::get(int32_t& int_value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      int_value = as_bool() ? 1 : 0;
      return true;
    case INT32:
      int_value = as_int32();
      return true;
    case INT64:
      int_value = static_cast<int32_t>(as_int64());
      return true;
    case DOUBLE:
      int_value = static_cast<int32_t>(as_double());
      return true;
    default:
      return false;
  }
}

bool Variant::get(int64_t& int64_value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      int64_value = as_bool() ? 1 : 0;
      return true;
    case INT32:
      int64_value = as_int32();
      return true;
    case INT64:
      int64_value = as_int64();
      return true;
    case DOUBLE:
      int64_value = static_cast<int64_t>(as_double());
      return true;
    default:
      return false;
  }
}

bool Variant::get(double& double_value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      double_value = as_bool() ? 1.0 : 0.0;
      return true;
    case INT32:
      double_value = static_cast<double>(as_int32());
      return true;
    case INT64:
      double_value = static_cast<double>(as_int64());
      return true;
    case DOUBLE:
      double_value = as_double();
      return true;
    default:
      return false;
  }
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
    case BOOL:
      string_value =
          FormatHelper<String>(as_bool() ? kTrueString : kFalseString);
      return true;
    case INT32:
      string_value = FormatHelper<String>(as_int32());
      return true;
    case INT64:
      string_value = FormatHelper<String>(as_int64());
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
    case EMPTY:
      string_value.clear();
      return true;
    default:
      return false;
  }
}

bool Variant::get(std::string& string_value) const {
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

NodeId Variant::get_or(NodeId&& or_value) const {
  NodeId result = std::move(or_value);
  get(result);
  return result;
}

bool Variant::ChangeType(Variant::Type new_type) {
  if (!is_scalar())
    return false;

  if (type() == new_type)
    return true;

  switch (new_type) {
    case BOOL:
      return ChangeTypeTo<bool>();
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
  static_assert(static_cast<size_t>(Type::COUNT) == 15);

  if (type() == Type::EXTENSION_OBJECT)
    return get<ExtensionObject>().data_type_id().node_id();

  const scada::NumericId kNodeIds[] = {
      0,          id::Boolean,        id::SByte,
      id::Byte,   id::Int32,          id::UInt32,
      id::Int64,  id::Double,         id::ByteString,
      id::String, id::QualifiedName,  id::LocalizedText,
      id::NodeId, id::ExpandedNodeId,
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
