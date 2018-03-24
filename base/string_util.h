#pragma once

#include "base/strings/string_piece.h"

inline bool IsEqualNoCase(base::StringPiece a, base::StringPiece b) {
  if (a.size() != b.size())
    return false;
  if (a.empty())
    return true;
  return std::equal(a.begin(), a.end(), b.begin(),
                    [](char a, char b) { return tolower(a) == tolower(b); });
}

inline bool IsEqualNoCase(base::StringPiece16 a, base::StringPiece16 b) {
  if (a.size() != b.size())
    return false;
  if (a.empty())
    return true;
  return std::equal(a.begin(), a.end(), b.begin(),
                    [](char a, char b) { return tolower(a) == tolower(b); });
}
