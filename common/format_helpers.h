#pragma once

#include "base/strings/stringprintf.h"
#include "base/format_time.h"
#include "core/data_value.h"

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
  return base::StringPrintf("value={%s}, qualifier={%s}, time={%s}, collection_time={%s}",
      ToString(value.value).c_str(), Format(value.qualifier).c_str(),
      FormatTime(value.source_timestamp).c_str(),
      FormatTime(value.server_timestamp).c_str());
}
