#pragma once

#include "base/win/scoped_variant.h"
#include "scada/variant.h"

#include <optional>

namespace scada::opc {

class VariantConverter {
 public:
  VariantConverter() = delete;

  static std::optional<base::win::ScopedVariant> ToWin(
      const scada::Variant& variant);

  static std::optional<scada::Variant> ToScada(const VARIANT& variant);
};

}  // namespace scada::opc
