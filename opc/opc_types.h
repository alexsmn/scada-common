#pragma once

#include "base/win/scoped_variant.h"

namespace opc {

struct OpcDataValue {
  HRESULT status = S_OK;
  base::win::ScopedVariant value;
  FILETIME timestamp = {};
  unsigned quality = 0;
};

}  // namespace opc
