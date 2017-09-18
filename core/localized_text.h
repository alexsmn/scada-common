#pragma once

#include "core/string.h"

namespace scada {

class LocalizedText {
 public:
  LocalizedText() {}
  LocalizedText(String text) : text_{std::move(text)} {}

  bool empty() const { return text_.empty(); }
  const String& text() const { return text_; }

  bool operator==(const LocalizedText& other) const { return false; }
  bool operator!=(const LocalizedText& other) const { return !operator==(other); }

  void clear() { text_.clear(); }

 private:
  String text_;
};

} // namespace scada