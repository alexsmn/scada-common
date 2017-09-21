#include "core/variant.h"

#include <cassert>
#include <limits>

#include "base/format.h"

namespace scada {

const char* Variant::kTrueString = "Да";
const char* Variant::kFalseString = "Нет";

void Variant::clear() {
  data_ = std::monostate{};
}

Variant& Variant::operator=(const Variant& source) {
  data_ = source.data_;
  return *this;
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

template<class String>
struct FormatHelper;

template<>
struct FormatHelper<String> {
  template<typename T>
  static String Format(T&& value) {
    return ::Format(std::forward<T>(value));
  }
};

template<>
struct FormatHelper<LocalizedText> {
  template<typename T>
  static LocalizedText Format(T&& value) {
    return ::Format(std::forward<T>(value));
  }
};

template<class String>
bool Variant::ToStringHelper(String& string_value) const {
  if (!is_scalar())
    return false;

  switch (type()) {
    case BOOL:
      string_value = FormatHelper<String>::Format(base::StringPiece(as_bool() ? kTrueString : kFalseString));
      return true;
    case INT32:
      string_value = FormatHelper<String>::Format(as_int32());
      return true;
    case INT64:
      string_value = FormatHelper<String>::Format(as_int64());
      return true;
    case DOUBLE:
      string_value = FormatHelper<String>::Format(as_double());
      return true;
    case STRING:
      string_value = FormatHelper<String>::Format(as_string());
      return true;
    case LOCALIZED_TEXT:
      string_value = FormatHelper<String>::Format(as_localized_text().text());
      return true;
    case NODE_ID:
      string_value = FormatHelper<String>::Format(as_node_id().ToString());
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
    return get<ExtensionObject>().data_type_id();

  const scada::NumericId kNodeIds[] = {
      0,
      OpcUaId_Boolean,
      OpcUaId_SByte,
      OpcUaId_Byte,
      OpcUaId_Int32,
      OpcUaId_UInt32,
      OpcUaId_Int64,
      OpcUaId_Double,
      OpcUaId_ByteString,
      OpcUaId_String,
      OpcUaId_QualifiedName,
      OpcUaId_LocalizedText,
      OpcUaId_NodeId,
      OpcUaId_ExpandedNodeId,
  };

  assert(static_cast<size_t>(type()) < std::size(kNodeIds));
  return kNodeIds[static_cast<size_t>(type())];
}

String ToString(const Variant& value) {
  return value.get_or(String{});
}

} // namespace scada
