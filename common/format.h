#pragma once

#include "common/value_format.h"
#include "scada/localized_text.h"
#include "scada/qualifier.h"
#include "scada/string.h"
#include "scada/variant.h"

#include <string>
#include <string_view>

namespace scada {
class NodeId;
class Variant;
}  // namespace scada

// The labels this library falls back to when a value carries none of its own.
enum class FallbackLabel {
  // The state labels of a two-state (TS) item whose TsFormat carries no
  // CloseLabel/OpenLabel.
  kDefaultClose,
  kDefaultOpen,
  // Placeholders for a value whose display name is empty or cannot be
  // resolved.
  kEmptyDisplayName,
  kUnknownDisplayName,
};

// Produces the operator-facing text of a fallback label, in the display
// locale.
using FallbackLabelProvider = std::u16string (*)(FallbackLabel label);

// Installs the provider the four functions below use. This library carries no
// operator-facing wording for them; the Qt client installs its catalog-backed
// words at startup. Without one — the server, and unit tests — they render
// their invariant forms: `1`/`0` for the state labels, the spreadsheet
// placeholder `#NAME?` for the display names. Pass nullptr to remove it.
void SetFallbackLabelProvider(FallbackLabelProvider provider);

// Looked up per call, so a provider installed after load takes effect.
std::u16string DefaultCloseLabel();
std::u16string DefaultOpenLabel();
std::u16string EmptyDisplayName();
std::u16string UnknownDisplayName();

std::string FormatFloat(double val, const char* fmt);

// TODO: Move to a separate file.
void EscapeColoredString(std::u16string& str);

bool StringToValue(std::string_view str,
                   scada::Variant::Type data_type,
                   scada::Variant& value);
bool StringToValue(std::u16string_view str,
                   scada::Variant::Type data_type,
                   scada::Variant& value);

struct TsFormatParams {
  scada::LocalizedText close_label;
  scada::LocalizedText open_label;
};

scada::LocalizedText FormatTs(bool bool_value,
                              const TsFormatParams& params = {});

struct TitFormatParams {
  scada::String display_format;
  scada::LocalizedText engineering_units;
};

scada::LocalizedText FormatTit(double double_value,
                               const TitFormatParams& params = {});
