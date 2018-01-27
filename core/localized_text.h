#pragma once

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

#define LOCALIZED_TEXT(text) L##text

namespace scada {

using LocalizedText = base::string16;

LocalizedText ToLocalizedText(base::StringPiece string);

inline const LocalizedText& ToLocalizedText(const base::string16& string) {
  return string;
}

inline LocalizedText ToLocalizedText(base::string16&& string) {
  return string;
}

}  // namespace scada

std::string ToString(const scada::LocalizedText& text);

inline const base::string16& ToString16(
    const scada::LocalizedText& localized_text) {
  return localized_text;
}
