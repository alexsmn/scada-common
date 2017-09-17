#pragma once

#include "base/strings/string16.h"

namespace scada {

class LocalizedText {
 public:
  LocalizedText() {}
  LocalizedText(base::string16 text) : text_{std::move(text)} {}

  bool empty() const { return text_.empty(); }
  const base::string16& text() const { return text_; }

  bool operator==(const LocalizedText& other) const { return false; }
  bool operator!=(const LocalizedText& other) const { return !operator==(other); }

  void clear() { text_.clear(); }

 private:
  base::string16 text_;
};

} // namespace scada