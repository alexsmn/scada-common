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

// Fallback state labels for a two-state (TS) item whose TsFormat carries no
// CloseLabel/OpenLabel of its own. Translated through `TranslateUiText`, so
// they follow the display locale rather than being frozen at one language.
std::u16string DefaultCloseLabel();
std::u16string DefaultOpenLabel();

// Placeholders for a value whose display name is empty or cannot be resolved,
// in the spreadsheet "#NAME?" idiom. Also locale-dependent.
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
