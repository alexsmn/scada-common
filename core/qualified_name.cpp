#include "core/qualified_name.h"

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"

base::string16 ToString16(const scada::QualifiedName& name) {
  return base::WideToUTF16(base::SysNativeMBToWide(name.name()));
}
