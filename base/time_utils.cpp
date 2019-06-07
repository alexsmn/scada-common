#include "base/time_utils.h"

#include "base/strings/stringprintf.h"

std::string SerializeToString(base::TimeDelta delta) {
  int64 s = delta.InSeconds();
  int64 m = s / 60;
  s = s % 60;
  int64 h = m / 60;
  m = m % 60;
  return base::StringPrintf("%d:%02d:%02d", static_cast<int>(h),
                            static_cast<int>(m), static_cast<int>(s));
}

bool Deserialize(base::StringPiece str, base::TimeDelta& delta) {
  int h, m, s;
  if (sscanf(str.as_string().c_str(), "%d:%d:%d", &h, &m, &s) != 3)
    return false;

  if (h < 0 || m < 0 || s < 0)
    return false;

  delta = base::TimeDelta::FromHours(h) + base::TimeDelta::FromMinutes(m) +
          base::TimeDelta::FromSeconds(s);
  return true;
}

std::string SerializeToString(base::Time time) {
  base::Time::Exploded e = {0};
  time.UTCExplode(&e);
  auto str =
      base::StringPrintf("%04d-%02d-%02d %02d:%02d:%02d", e.year, e.month,
                         e.day_of_month, e.hour, e.minute, e.second);

  if (e.millisecond != 0)
    str += base::StringPrintf(".%03d", e.millisecond);

#ifndef NDEBUG
  base::Time parsed_time;
  bool parse_result = Deserialize(str, parsed_time);
  assert(parse_result);
  assert(time == parsed_time);
#endif

  return str;
}

bool Deserialize(base::StringPiece str, base::Time& time) {
  return base::Time::FromUTCString(str.as_string().c_str(), &time);
}
