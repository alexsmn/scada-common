#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "base/format_time.h"

std::string FormatTime(base::Time time, int flags) {
  if (time.is_null())
    return std::string();

  base::Time::Exploded e = {0};
  if (flags & TIME_FORMAT_UTC)
    time.UTCExplode(&e);
  else
    time.LocalExplode(&e);

  std::string text;

  if (flags & TIME_FORMAT_DATE)
    text = base::StringPrintf("%02d.%02d.%04d", e.day_of_month, e.month, e.year);

  if (flags & TIME_FORMAT_TIME) {
    std::string tmp = base::StringPrintf("%02d:%02d:%02d", e.hour, e.minute,
        e.second);
    if (!text.empty())
      text += L' ';
    text += tmp;
    if (flags & TIME_FORMAT_MSEC)
      text += base::StringPrintf(".%03d", e.millisecond);
  }

  return text;
}
