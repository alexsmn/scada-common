#pragma once

#include <ostream>
#include <vector>

template <class A, class B>
inline std::ostream& operator<<(std::ostream& stream,
                                const std::pair<A, B>& pair) {
  return stream << "{"
                << "first: " << pair.first << ", "
                << "second: " << pair.second
                << "}";
}

template <class T>
inline std::ostream& operator<<(std::ostream& stream, const std::vector<T>& v) {
  stream << "[";
  for (size_t i = 0; i < v.size(); ++i) {
    stream << v[i];
    if (i != v.size() - 1)
      stream << ", ";
  }
  stream << "]";
  return stream;
}
