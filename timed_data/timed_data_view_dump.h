#pragma once

#include <map>

namespace internal {

// Should be even.
const size_t kDumpMaxValueCount = 10;

template <class T>
inline void Dump(std::ostream& stream, const T& value) {
  stream << value;
}

inline void Dump(std::ostream& stream, scada::DateTime time) {
  if (time == scada::base::kMinTime)
    stream << "min";
  else if (time == scada::base::kMaxTime)
    stream << "max";
  else if (scada::base::IsNull(time))
    stream << "null";
  else
    stream << time;
}

inline void Dump(std::ostream& stream, const scada::DateTimeRange& range) {
  stream << "(";
  Dump(stream, range.first);
  stream << ",";
  Dump(stream, range.second);
  stream << ")";
}

inline void Dump(std::ostream& stream,
                 const std::map<TimedDataViewObserver*, scada::DateTimeRange>&
                     observer_ranges) {
  for (auto& p : observer_ranges) {
    Dump(stream, p.second);
    stream << std::endl;
  }
}

template <class It>
inline void DumpRange(std::ostream& stream, It first, It last) {
  for (auto i = first; i != last; ++i) {
    Dump(stream, *i);
    stream << std::endl;
  }
}

template <class T>
inline void Dump(std::ostream& stream, const std::vector<T>& v) {
  DumpRange(stream, v.begin(), v.end());
}

}  // namespace internal
