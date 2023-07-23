#pragma once

#include "scada/localized_text.h"
#include "scada/qualifier.h"
#include "scada/string.h"

#include <string>
#include <string_view>

namespace scada {
class NodeId;
class Variant;
}  // namespace scada

extern const char16_t kDefaultCloseLabel[];
extern const char16_t kDefaultOpenLabel[];

extern const char16_t kEmptyDisplayName[];
extern const char16_t kUnknownDisplayName[];

enum FormatFlags {
  FORMAT_QUALITY = 0x0001,
  FORMAT_STATUS = 0x0002,  // manual & locked modifiers
  FORMAT_UNITS = 0x0004,
  FORMAT_COLOR = 0x0008,

  FORMAT_DEFAULT = FORMAT_QUALITY | FORMAT_STATUS | FORMAT_UNITS
};

std::string FormatFloat(double val, const char* fmt);

void EscapeColoredString(std::u16string& str);

bool StringToValue(std::string_view str,
                   const scada::NodeId& data_type_id,
                   scada::Variant& value);
bool StringToValue(std::u16string_view str,
                   const scada::NodeId& data_type_id,
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
