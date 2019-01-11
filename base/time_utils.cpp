#include "base/time_utils.h"

#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"

std::string FormatTimeDelta(base::TimeDelta delta) {
  int64 s = delta.InSeconds();
  int64 m = s / 60;
  s = s % 60;
  int64 h = m / 60;
  m = m % 60;
  return base::StringPrintf("%d:%02d:%02d", static_cast<int>(h),
                            static_cast<int>(m), static_cast<int>(s));
}

bool ParseTimeDelta(base::StringPiece str, base::TimeDelta& delta) {
  int h, m, s;
  if (sscanf(str.as_string().c_str(), "%d:%d:%d", &h, &m, &s) != 3)
    return false;

  if (h < 0 || m < 0 || s < 0)
    return false;

  delta = base::TimeDelta::FromHours(h) + base::TimeDelta::FromMinutes(m) +
          base::TimeDelta::FromSeconds(s);
  return true;
}

base::string16 FormatTime16(base::Time time) {
  base::Time::Exploded e = {0};
  time.UTCExplode(&e);
  return base::StringPrintf(L"%02d-%02d-%04d %02d:%02d:%02d.%03d",
                            e.day_of_month, e.month, e.year, e.hour, e.minute,
                            e.second, e.millisecond);
}

bool ParseTime(base::StringPiece str, base::Time& time) {
  int d, m, y, h, n, s, ms;
  if (sscanf_s(str.as_string().c_str(), "%02d-%02d-%04d %02d:%02d:%02d.%03d",
               &d, &m, &y, &h, &n, &s, &ms) != 7)
    return false;

  base::Time::Exploded e = {0};
  e.year = y;
  e.month = m;
  e.day_of_month = d;
  e.hour = h;
  e.minute = n;
  e.second = s;
  e.millisecond = ms;

  base::Time t;
  if (!base::Time::FromUTCExploded(e, &t))
    return false;

  time = t;
  return true;
}
