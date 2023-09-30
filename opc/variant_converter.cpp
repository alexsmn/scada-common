#include "opc/variant_converter.h"

#include "base/strings/string_util.h"

namespace opc {

namespace {

inline std::optional<scada::Variant> ScalarToVariant(const VARIANT& value) {
  switch (value.vt) {
    case VT_EMPTY:
      return scada::Variant{};
    case VT_BOOL:
      return value.boolVal != VARIANT_FALSE;
    case VT_I4:
      return value.intVal;
    case VT_UI4:
      return value.uintVal;
    case VT_R4:
      return value.fltVal;
    case VT_R8:
      return value.dblVal;
    case VT_BSTR:
      return scada::LocalizedText{base::AsString16(value.bstrVal)};
    default:
      assert(false);
      return std::nullopt;
  }
}

inline std::optional<scada::Variant> SafeArrayToVariant(
    const SAFEARRAY& safe_array) {
  return std::nullopt;
}

}  // namespace

// VariantConverter

// static
std::optional<base::win::ScopedVariant> VariantConverter::Convert(
    const scada::Variant& value) {
  switch (value.type()) {
    case scada::Variant::EMPTY:
      return base::win::ScopedVariant{};
    case scada::Variant::BOOL:
      return base::win::ScopedVariant{value.as_bool()};
    case scada::Variant::INT8:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::Int8>()), VT_I1};
    case scada::Variant::UINT8:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::UInt8>()), VT_UI1};
    case scada::Variant::INT16:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::Int16>()), VT_I2};
    case scada::Variant::UINT16:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::UInt16>()), VT_UI2};
    case scada::Variant::INT32:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::Int32>()), VT_I4};
    case scada::Variant::UINT32:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::UInt32>()), VT_UI4};
    case scada::Variant::INT64:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::Int64>()), VT_I8};
    case scada::Variant::UINT64:
      return base::win::ScopedVariant{
          static_cast<long>(value.get<scada::UInt64>()), VT_UI8};
    case scada::Variant::DOUBLE:
      return base::win::ScopedVariant{value.get<scada::Double>(), VT_R8};
    default:
      assert(false);
      return std::nullopt;
  }
}

// static
std::optional<scada::Variant> VariantConverter::Convert(const VARIANT& value) {
  if (value.vt & VT_ARRAY) {
    return SafeArrayToVariant(*value.parray);
  }

  return ScalarToVariant(value);
}

}  // namespace opc
