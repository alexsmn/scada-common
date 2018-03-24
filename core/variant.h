#pragma once

#include "core/basic_types.h"
#include "core/expanded_node_id.h"
#include "core/extension_object.h"
#include "core/localized_text.h"
#include "core/node_id.h"
#include "core/qualified_name.h"
#include "core/string.h"

#include <cstdint>
#include <variant>

namespace scada {

class Variant {
 public:
  enum Type {
    EMPTY,
    BOOL,
    INT8,
    UINT8,
    INT32,
    UINT32,
    INT64,
    DOUBLE,
    BYTE_STRING,
    STRING,
    QUALIFIED_NAME,
    LOCALIZED_TEXT,
    NODE_ID,
    EXPANDED_NODE_ID,
    EXTENSION_OBJECT,
    COUNT
  };

  Variant() {}
  Variant(bool value) : data_{value} {}
  Variant(int8_t value) : data_{value} {}
  Variant(uint8_t value) : data_{value} {}
  Variant(int32_t value) : data_{value} {}
  Variant(uint32_t value) : data_{value} {}
  Variant(int64_t value) : data_{value} {}
  Variant(double value) : data_{value} {}
  Variant(ByteString str) : data_{std::move(str)} {}
  Variant(String str) : data_{std::move(str)} {}
  Variant(QualifiedName value) : data_{std::move(value)} {}
  Variant(LocalizedText str) : data_{std::move(str)} {}
  Variant(const char* str) : data_{str ? String{str} : String{}} {}
  Variant(const wchar_t* str)
      : data_{str ? LocalizedText{str} : LocalizedText{}} {}
  Variant(NodeId node_id) : data_{std::move(node_id)} {}
  Variant(ExpandedNodeId node_id) : data_{std::move(node_id)} {}
  Variant(ExtensionObject source) : data_{std::move(source)} {}
  Variant(std::vector<String> value) : data_{std::move(value)} {}
  Variant(std::vector<LocalizedText> value) : data_{std::move(value)} {}
  Variant(std::vector<ExtensionObject> value) : data_{std::move(value)} {}

  Variant(const Variant& source) : data_{source.data_} {}
  Variant(Variant&& source) : data_{std::move(source.data_)} {}

  ~Variant() { clear(); }

  void clear();

  bool is_null() const { return type() == EMPTY; }

  Type type() const;
  bool is_scalar() const;
  bool is_array() const { return !is_scalar(); }

  NodeId data_type_id() const;

  bool as_bool() const { return std::get<bool>(data_); }
  int32_t as_int32() const { return std::get<int32_t>(data_); }
  int64_t as_int64() const { return std::get<int64_t>(data_); }
  double as_double() const { return std::get<double>(data_); }
  const String& as_string() const { return std::get<String>(data_); }
  const LocalizedText& as_localized_text() const {
    return std::get<LocalizedText>(data_);
  }
  const NodeId& as_node_id() const { return std::get<NodeId>(data_); }

  template <class T>
  const T& get() const {
    return std::get<T>(data_);
  }
  template <class T>
  T& get() {
    return std::get<T>(data_);
  }

  bool get(bool& bool_value) const;
  bool get(int32_t& int_value) const;
  bool get(int64_t& int64_value) const;
  bool get(double& double_value) const;
  bool get(std::string& string_value) const;
  bool get(QualifiedName& value) const;
  bool get(LocalizedText& value) const;
  bool get(NodeId& node_id) const;

  bool get_or(bool or_value) const;
  int32_t get_or(int32_t or_value) const;
  double get_or(double or_value) const;
  std::string get_or(std::string&& or_value) const;
  QualifiedName get_or(QualifiedName&& or_value) const;
  LocalizedText get_or(LocalizedText&& or_value) const;
  NodeId get_or(NodeId&& or_value) const;

  template <class T>
  const T* get_if() const;

  Variant& operator=(const Variant& source) = default;
  Variant& operator=(Variant&& source);

  bool operator==(const Variant& other) const;
  bool operator!=(const Variant& other) const { return !operator==(other); }

  bool ChangeType(Variant::Type new_type);
  template <typename T>
  bool ChangeTypeTo();

  static const wchar_t* kTrueString;
  static const wchar_t* kFalseString;

 private:
  template <class String>
  bool ToStringHelper(String& string_value) const;

  struct Placeholder : std::monostate {};

  std::variant<std::monostate,
               bool,
               int8_t,
               uint8_t,
               int32_t,
               uint32_t,
               int64_t,
               double,
               ByteString,
               String,
               QualifiedName,
               LocalizedText,
               NodeId,
               ExpandedNodeId,
               ExtensionObject,
               Placeholder,
               std::vector<bool>,
               std::vector<int8_t>,
               std::vector<uint8_t>,
               std::vector<int32_t>,
               std::vector<uint32_t>,
               std::vector<int64_t>,
               std::vector<double>,
               std::vector<ByteString>,
               std::vector<String>,
               std::vector<QualifiedName>,
               std::vector<LocalizedText>,
               std::vector<NodeId>,
               std::vector<ExpandedNodeId>,
               std::vector<ExtensionObject> >
      data_;
};

inline Variant::Type Variant::type() const {
  if (data_.index() < static_cast<size_t>(Type::COUNT))
    return static_cast<Type>(data_.index());
  else
    return static_cast<Type>(data_.index() - static_cast<size_t>(Type::COUNT));
}

inline bool Variant::is_scalar() const {
  return data_.index() < static_cast<size_t>(Type::COUNT);
}

template <class T>
inline bool Variant::ChangeTypeTo() {
  T value;
  if (!get(value))
    return false;
  data_ = std::move(value);
  return true;
}

template <class T>
inline const T* Variant::get_if() const {
  return std::get_if<T>(&data_);
}

}  // namespace scada

std::string ToString(const scada::Variant& value);
base::string16 ToString16(const scada::Variant& value);

inline std::ostream& operator<<(std::ostream& stream, const scada::Variant& v) {
  return stream << v.get_or("(error)");
}
