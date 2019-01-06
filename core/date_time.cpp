#include "core/date_time.h"

#include "base/format_time.h"
#include "base/strings/utf_string_conversions.h"

std::string ToString(scada::DateTime time) {
  return FormatTime(time);
}

base::string16 ToString16(scada::DateTime time) {
  return base::ASCIIToUTF16(FormatTime(time));
}

/*std::ostream& operator<<(std::ostream& stream, scada::DateTime time) {
  return stream << FormatTime(time);
}*/
