#include "localized_text.h"

#include "base/strings/sys_string_conversions.h"

namespace scada {

LocalizedText ToLocalizedText(base::StringPiece string) {
  return string.as_string();
}

LocalizedText ToLocalizedText(const std::wstring& string) {
  return base::SysWideToNativeMB(string);
}

}  // namespace scada

namespace base {

string16 ToString16(const scada::LocalizedText& localized_text) {
  return SysNativeMBToWide(localized_text.text());
}

}  // namespace base