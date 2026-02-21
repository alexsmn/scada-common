#pragma once

#include "base/format_time.h"
#include "scada/data_value.h"

#include <format>

inline std::string Format(scada::Qualifier qualifier) {
  std::string str;
  if (qualifier.bad())
    str += "BAD+";
  if (qualifier.manual())
    str += "MANUAL+";
  if (qualifier.backup())
    str += "BACKUP+";
  if (qualifier.offline())
    str += "OFFLINE+";  
  if (qualifier.failed())
    str += "FAILED+";  
  if (qualifier.stale())
    str += "STALE+";  
  if (qualifier.simulated())
    str += "SIMULATED+";  
  if (qualifier.misconfigured())
    str += "MISCONFIGURED+";
  if (qualifier.sporadic())
    str += "SPORADIC+";
  if (!str.empty())
    str.erase(--str.end());
  return str;
}

inline std::string Format(const scada::DataValue& value) {
  return std::format("value={{{}}}, qualifier={{{}}}, time={{{}}}, collection_time={{{}}}",
      ToString(value.value), Format(value.qualifier),
      FormatTime(value.source_timestamp),
      FormatTime(value.server_timestamp));
}
