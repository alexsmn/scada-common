#pragma once

#include <ObjBase.h>
#include <string_view>

// TODO: Move everything to `opc_client/core/opc_util.h`.

namespace opc {

inline std::wstring GuidToString(const GUID& guid) {
  LPOLESTR string = nullptr;
  if (FAILED(::StringFromCLSID(guid, &string))) {
    assert(false);
    return {};
  }
  std::wstring result{string};
  ::CoTaskMemFree(string);
  return result;
}

inline LPOLESTR CreateOleString(std::wstring_view str) {
  LPOLESTR result = static_cast<LPOLESTR>(
      ::CoTaskMemAlloc(sizeof(OLECHAR) * (str.size() + 1)));
  if (result == nullptr) {
    return nullptr;
  }
  std::ranges::copy(str, result);
  result[str.size()] = L'\0';
  return result;
}

}  // namespace opc
