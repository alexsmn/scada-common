#pragma once

#include "base/strings/string_piece.h"
#include "core/qualifier.h"

#include <string>

namespace scada {
class NodeId;
class Variant;
}  // namespace scada

extern const wchar_t* kEmptyDisplayName;
extern const wchar_t* kUnknownDisplayName;

extern const wchar_t* kDefaultCloseLabel;
extern const wchar_t* kDefaultOpenLabel;

enum FormatFlags {
  FORMAT_QUALITY = 0x0001,
  FORMAT_STATUS = 0x0002,  // manual & locked modifiers
  FORMAT_UNITS = 0x0004,
  FORMAT_COLOR = 0x0008,

  FORMAT_DEFAULT = FORMAT_QUALITY | FORMAT_STATUS | FORMAT_UNITS
};

std::string FormatFloat(double val, const char* fmt);

void EscapeColoredString(std::wstring& str);

bool StringToValue(base::StringPiece str,
                   const scada::NodeId& data_type_id,
                   scada::Variant& value);
bool StringToValue(base::StringPiece16 str,
                   const scada::NodeId& data_type_id,
                   scada::Variant& value);
