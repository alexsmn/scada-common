#pragma once

#include "core/variant.h"

namespace scada {

template<class ValueType>
inline bool ConvertVariant(const Variant& source, ValueType& target) {
  return source.get(target);
}

template<> inline bool ConvertVariant(const Variant& source, Variant& target) { target = source; return true; }

} // namespace scada
