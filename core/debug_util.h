#pragma once

#include <ostream>
#include <vector>

template <class A, class B>
inline std::ostream& operator<<(std::ostream& stream,
                                const std::pair<A, B>& pair) {
  return stream << "{" << pair.first << ", " << pair.second << "}";
}

template <class A, class B>
inline std::string ToString(const std::pair<A, B>& pair) {
  std::string str = "{";
  str += ToString(pair.first);
  str += ", ";
  str += ToString(pair.second);
  str += "}";
  return str;
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

template <class T>
inline std::string ToString(const std::vector<T>& v) {
  std::string result = "[";
  if (!v.empty()) {
    result += ToString(v.front());
    for (size_t i = 1; i < v.size(); ++i) {
      result += ", ";
      result += ToString(v[i]);
    }
  }
  result += ']';
  return result;
}
