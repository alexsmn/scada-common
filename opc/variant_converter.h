#pragma once

#include "scada/variant.h"

#include <atlcomcli.h>
#include <optional>

namespace opc {

class VariantConverter {
 public:
  VariantConverter() = delete;

  static std::optional<CComVariant> Convert(const scada::Variant& variant);
  static std::optional<scada::Variant> Convert(const VARIANT& variant);
};

}  // namespace opc
