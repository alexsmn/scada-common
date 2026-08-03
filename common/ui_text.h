#pragma once

#include <string>
#include <string_view>

namespace scada {

// Translates an English UI source string into the display locale. The
// translation catalogs live in the UI layer (the client's Qt/Wt `Translate()`
// over the `.ts` files), which is layered above `scada_common`; this seam lets
// shared formatting code carry English literals — as the no-Cyrillic-in-source
// convention requires — without a compile-time dependency on that layer.
// Mirrors `scada::SetNamespaceNameResolver` in `core/scada/node_id_log.h`.
using UiTextTranslator = std::u16string (*)(std::string_view english);

// Installs the translator used by `TranslateUiText`. The client installs its
// `Translate()` at startup (see `AppInit`). Without it, `TranslateUiText`
// returns the English source text, which is what the server and unit tests
// want — neither ships translation catalogs.
void SetUiTextTranslator(UiTextTranslator translator);

// Translates `english` for display, falling back to `english` itself when no
// translator is installed.
std::u16string TranslateUiText(std::string_view english);

}  // namespace scada
