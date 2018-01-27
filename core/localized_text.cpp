#include "core/localized_text.h"

#include "base/strings/sys_string_conversions.h"

namespace scada {

LocalizedText ToLocalizedText(base::StringPiece string) {
  return base::SysNativeMBToWide(string);
}

}  // namespace scada

std::string ToString(const scada::LocalizedText& text) {
  return base::SysWideToNativeMB(text);
}
