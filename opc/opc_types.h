#pragma once

#include <atlcomcli.h>

namespace opc {

struct OpcDataValue {
  HRESULT status = S_OK;
  CComVariant value;
  FILETIME timestamp = {};
  unsigned quality = 0;
};

}  // namespace opc
