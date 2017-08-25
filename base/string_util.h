#pragma once

#include "base/strings/string_piece.h"

inline bool IsEqualNoCase(const base::StringPiece& a, const base::StringPiece& b) {
  if (a.size() != b.size())
    return false;
  if (a.empty())
    return true;
  return std::equal(a.begin(), a.end(), b.begin(), [](char a, char b) { return tolower(a) == tolower(b); });
}

inline bool IsEqualNoCase(const base::StringPiece16& a, const base::StringPiece16& b) {
  if (a.size() != b.size())
    return false;
  if (a.empty())
    return true;
  return std::equal(a.begin(), a.end(), b.begin(), [](char a, char b) { return tolower(a) == tolower(b); });
}
