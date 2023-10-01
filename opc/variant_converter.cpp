#include "opc/variant_converter.h"

#include "base/strings/string_util.h"

namespace opc {

namespace {

// `base::win::ScopedVariant` cannot directly initialize from many types, while
// its `Set` supports different types.
template <class T>
inline base::win::ScopedVariant MakeWinVariant(T value) {
  base::win::ScopedVariant v;
  v.Set(value);
  return v;
}

std::optional<base::win::ScopedVariant> ConvertScalar(
    const scada::Variant& value) {
  switch (value.type()) {
    case scada::Variant::EMPTY:
      return base::win::ScopedVariant{};
    case scada::Variant::BOOL:
      return MakeWinVariant(value.as_bool());
    case scada::Variant::INT8:
      return MakeWinVariant(value.get<scada::Int8>());
    case scada::Variant::UINT8:
      return MakeWinVariant(value.get<scada::UInt8>());
    case scada::Variant::INT16:
      return MakeWinVariant(value.get<scada::Int16>());
    case scada::Variant::UINT16:
      return MakeWinVariant(value.get<scada::UInt16>());
    case scada::Variant::INT32:
      return MakeWinVariant(value.get<scada::Int32>());
    case scada::Variant::UINT32:
      return MakeWinVariant(value.get<scada::UInt32>());
    case scada::Variant::INT64:
      return MakeWinVariant(value.get<scada::Int64>());
    case scada::Variant::UINT64:
      return MakeWinVariant(value.get<scada::UInt64>());
    case scada::Variant::DOUBLE:
      return MakeWinVariant(value.get<scada::Double>());
    default:
      assert(false);
      return std::nullopt;
  }
}

std::optional<scada::Variant> ConvertScalar(const VARIANT& value) {
  switch (value.vt) {
    case VT_EMPTY:
      return scada::Variant{};
    case VT_BOOL:
      return scada::Variant{value.boolVal != VARIANT_FALSE};
    case VT_I1:
      return scada::Variant{static_cast<scada::Int8>(value.bVal)};
    case VT_UI1:
      return scada::Variant{static_cast<scada::UInt8>(value.cVal)};
    case VT_I2:
      return scada::Variant{static_cast<scada::Int16>(value.iVal)};
    case VT_UI2:
      return scada::Variant{static_cast<scada::UInt16>(value.uiVal)};
    case VT_I4:
      return scada::Variant{static_cast<scada::Int32>(value.intVal)};
    case VT_UI4:
      return scada::Variant{static_cast<scada::UInt32>(value.uintVal)};
    case VT_I8:
      return scada::Variant{static_cast<scada::Int64>(value.llVal)};
    case VT_UI8:
      return scada::Variant{static_cast<scada::UInt64>(value.ullVal)};
    case VT_R4:
      return scada::Variant{static_cast<scada::Double>(value.fltVal)};
    case VT_R8:
      return scada::Variant{static_cast<scada::Double>(value.dblVal)};
    case VT_BSTR:
      return scada::Variant{
          scada::LocalizedText{base::AsString16(value.bstrVal)}};
    default:
      assert(false);
      return std::nullopt;
  }
}

std::optional<base::win::ScopedVariant> ConvertArray(
    const scada::Variant& variant) {
  // TODO: Implement.
  assert(false);
  return std::nullopt;
}

std::optional<scada::Variant> ConvertArray(const SAFEARRAY& safe_array) {
  // TODO: Implement.
  assert(false);
  return std::nullopt;
}

}  // namespace

// VariantConverter

// static
std::optional<base::win::ScopedVariant> VariantConverter::Convert(
    const scada::Variant& value) {
  if (value.is_array()) {
    return ConvertArray(value);
  }

  return ConvertScalar(value);
}

// static
std::optional<scada::Variant> VariantConverter::Convert(const VARIANT& value) {
  if (value.vt & VT_ARRAY) {
    return ConvertArray(*value.parray);
  }

  return ConvertScalar(value);
}

}  // namespace opc
