#pragma once

#include <optional>
#include <ostream>
#include <vector>

#include "base/strings/string16.h"

template <class A, class B>
std::ostream& operator<<(std::ostream& stream,
                                const std::pair<A, B>& pair);

template <class A, class B>
std::string ToString(const std::pair<A, B>& pair);

template <class T>
std::ostream& operator<<(std::ostream& stream, const std::vector<T>& v);

template <class T>
std::string ToString(const std::vector<T>& v);

template <class T>
std::ostream& operator<<(std::ostream& stream, const std::optional<T>& v);

template <class T>
std::string ToString(const std::optional<T>& v);

template <class T>
base::string16 ToString16(const std::optional<T>& v);
