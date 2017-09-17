#pragma once

#include "base/strings/string_piece.h"
#include "base/strings/string16.h"
#include "core/extension_object.h"
#include "core/node_id.h"
#include "core/qualified_name.h"
#include "core/string.h"

#include <boost/variant.hpp>
#include <cstdint>
#include <opcua_builtintypes.h>
#include <vector>

namespace scada {

using LocalizedText = base::string16;
using ByteString = std::vector<char>;

class Variant {
 public:
  enum Type { EMPTY, BOOL, INT8, UINT8, INT32, UINT32, INT64, DOUBLE, BYTE_STRING, STRING, QUALIFIED_NAME, LOCALIZED_TEXT, NODE_ID, EXTENSION_OBJECT,
      VECTOR_STRING, VECTOR_LOCALIZED_TEXT, VECTOR_EXTENSION_OBJECT, COUNT };

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
  Variant(NodeId node_id) : data_{std::move(node_id)} {}
  Variant(ExtensionObject source) : data_{std::move(source)} {}
  Variant(std::vector<String> value) : data_{std::move(value)} {}
  Variant(std::vector<LocalizedText> value) : data_{std::move(value)} {}
  Variant(std::vector<ExtensionObject> value) : data_{std::move(value)} {}

  Variant(const Variant& source) : data_{source.data_} {}
  Variant(Variant&& source) : data_{std::move(source.data_)} {}

  ~Variant() { clear(); }

  void clear();

  bool is_null() const { return type() == EMPTY; }
  Type type() const { return static_cast<Type>(data_.which()); }

  bool as_bool() const { return boost::get<bool>(data_); }
  int32_t as_int32() const { return boost::get<int32_t>(data_); }
  int64_t as_int64() const  { return boost::get<int64_t>(data_); }
  double as_double() const  { return boost::get<double>(data_); }
  const String& as_string() const  { return boost::get<String>(data_); }
  const LocalizedText& as_string16() const  { return boost::get<LocalizedText>(data_); }
  const NodeId& as_node_id() const  { return boost::get<NodeId>(data_); }

  template<typename T> const T& get() const { return boost::get<T>(data_); }
  template<typename T> T& get() { return boost::get<T>(data_); }

  bool get(bool& bool_value) const;
  bool get(int32_t& int_value) const;
  bool get(int64_t& int64_value) const;
  bool get(double& double_value) const;
  bool get(std::string& string_value) const;
  bool get(base::StringPiece& string_piece_value) const;
  bool get(base::string16& string_value) const;
  bool get(base::StringPiece16& string_piece_value) const;
  bool get(NodeId& node_id) const;

  bool get_or(bool or_value) const;
  int32_t get_or(int32_t or_value) const;
  double get_or(double or_value) const;
  std::string get_or(std::string&& or_value) const;
  base::StringPiece get_or(base::StringPiece&& or_value) const;
  base::string16 get_or(base::string16&& or_value) const;
  base::StringPiece16 get_or(base::StringPiece16&& or_value) const;
  NodeId get_or(NodeId&& or_value) const;

  Variant& operator=(const Variant& source);
  Variant& operator=(Variant&& source);

  bool operator==(const Variant& other) const;
  bool operator!=(const Variant& other) const { return !operator==(other); }

  bool ChangeType(Variant::Type new_type);
  template<typename T> bool ChangeTypeTo();

  static const char* kTrueString;
  static const char* kFalseString;

 private:
  template<class String>
  bool ToStringHelper(String& string_value) const;

  boost::variant<
      boost::blank, bool, int8_t, uint8_t, int32_t, uint32_t, int64_t, double, ByteString, String, QualifiedName, LocalizedText, NodeId, ExtensionObject,
      std::vector<String>,
      std::vector<LocalizedText>,
      std::vector<ExtensionObject>
  > data_;
};

template<typename T>
inline bool Variant::ChangeTypeTo() {
  T value;
  if (!get(value))
    return false;
  data_ = std::move(value);
  return true;
}

String ToString(const Variant& value);

} // namespace scada
