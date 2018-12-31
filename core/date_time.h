#pragma once

#include "base/strings/string16.h"
#include "base/time/time.h"

#include <ostream>

namespace scada {

using DateTime = base::Time;
using DateTimeRange = std::pair<DateTime, DateTime>;

using Duration = base::TimeDelta;

}  // namespace scada

std::string ToString(scada::DateTime time);
base::string16 ToString16(scada::DateTime time);

std::ostream& operator<<(std::ostream& stream, scada::DateTime time);
