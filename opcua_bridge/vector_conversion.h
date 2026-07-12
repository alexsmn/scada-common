#pragma once

// Element-wise std::vector converters. Included LAST (after all ToOpcua/ToScada
// overloads are declared) so the ordinary unqualified lookup inside these
// templates sees every element converter — the element types live in scada::
// and opcua::, not opcua_bridge::, so ADL alone would not find them.

#include "opcua/types/status_or.h"
#include "scada/status_or.h"

#include <type_traits>
#include <vector>

#include "opcua_bridge/opcua_bridge_compat.h"
namespace scada::opcua_bridge {

template <class T>
auto ToOpcuaVector(const std::vector<T>& in) {
  std::vector<std::decay_t<decltype(ToOpcua(in.front()))>> out;
  out.reserve(in.size());
  for (const auto& x : in)
    out.push_back(ToOpcua(x));
  return out;
}

template <class T>
auto ToScadaVector(const std::vector<T>& in) {
  std::vector<std::decay_t<decltype(ToScada(in.front()))>> out;
  out.reserve(in.size());
  for (const auto& x : in)
    out.push_back(ToScada(x));
  return out;
}

// StatusOr<vector<T>> — propagate the status on failure, convert the vector on
// success. (Covers every StatusOr the service interfaces return.)
template <class T>
auto ToOpcua(const scada::StatusOr<std::vector<T>>& s) -> opcua::StatusOr<
    std::vector<std::decay_t<decltype(ToOpcua(std::declval<T>()))>>> {
  if (!s.ok())
    return ToOpcua(s.status());
  return ToOpcuaVector(*s);
}
template <class T>
auto ToScada(const opcua::StatusOr<std::vector<T>>& s) -> scada::StatusOr<
    std::vector<std::decay_t<decltype(ToScada(std::declval<T>()))>>> {
  if (!s.ok())
    return ToScada(s.status());
  return ToScadaVector(*s);
}

}  // namespace scada::opcua_bridge
