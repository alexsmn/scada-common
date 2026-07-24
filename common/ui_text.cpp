#include "common/ui_text.h"

#include "base/utf_convert.h"

namespace scada {

namespace {

// Set once at startup by the UI layer and read thereafter; a function-local
// static keeps it out of static-global-init ordering (matching the project's
// no-static-globals rule and `SetNamespaceNameResolver`).
UiTextTranslator& GetUiTextTranslator() {
  static UiTextTranslator translator = nullptr;
  return translator;
}

}  // namespace

void SetUiTextTranslator(UiTextTranslator translator) {
  GetUiTextTranslator() = translator;
}

std::u16string TranslateUiText(std::string_view english) {
  if (const UiTextTranslator translator = GetUiTextTranslator())
    return translator(english);
  return UtfConvert<char16_t>(english);
}

}  // namespace scada
