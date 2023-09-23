#pragma once

namespace internal {

// Should be even.
const size_t kDumpMaxValueCount = 10;

template <class T>
inline void Dump(std::ostream& stream, const std::vector<T>& v) {
  for (auto& e : v)
    stream << e << std::endl;
}

inline void Dump(std::ostream& stream, scada::DateTime time) {
  if (time.is_min())
    stream << "min";
  else if (time.is_max())
    stream << "max";
  else if (time.is_null())
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

inline void Dump(
    std::ostream& stream,
    const std::map<TimedDataObserver*, scada::DateTimeRange>& observer_ranges) {
  for (auto& p : observer_ranges) {
    Dump(stream, p.second);
    stream << std::endl;
  }
}

template <class It>
inline void DumpRange(std::ostream& stream, It first, It last) {
  for (auto i = first; i != last; ++i)
    stream << *i << std::endl;
}

}  // namespace internal
