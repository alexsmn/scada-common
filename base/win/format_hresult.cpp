#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/win/format_hresult.h"

#include <cassert>
#include <windows.h>

base::string16 FormatHresultUTF16(HRESULT err) {
  wchar_t* buf = NULL;
  if (!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM,
                      NULL, err, 0, reinterpret_cast<LPWSTR>(&buf), 0, NULL))
    return base::StringPrintf(L"Ошибка %d", err);

  assert(buf);

  // Delete ending line breaks.
  size_t len = wcslen(buf);
  while (len && (buf[len - 1] == L'\r' ||
                 buf[len - 1] == L'\n' ||
                 buf[len - 1] == L'.')) {
    len--;
  }

  base::string16 str(buf, len);
  LocalFree(buf);
  return str;
}

std::string FormatHresult(HRESULT err) {
  return base::SysWideToNativeMB(FormatHresultUTF16(err));
}
