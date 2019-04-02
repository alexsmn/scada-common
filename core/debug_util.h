#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include "base/strings/string16.h"

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

template <class T>
inline std::ostream& operator<<(std::ostream& stream,
                                const std::optional<T>& v) {
  if (v.has_value())
    stream << *v;
  else
    stream << "nullopt";
  return stream;
}

template <class T>
inline std::string ToString(const std::optional<T>& v) {
  return v.has_value() ? ToString(*v) : "nullopt";
}

template <class T>
inline base::string16 ToString16(const std::optional<T>& v) {
  return v.has_value() ? ToString16(*v) : L"nullopt";
}
