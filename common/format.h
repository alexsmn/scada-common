#pragma once

#include "base/strings/string_piece.h"
#include "base/strings/string16.h"
#include "core/qualifier.h"

namespace scada {
class NodeId;
class Variant;
}

extern const char* kDefaultCloseLabel;
extern const char* kDefaultOpenLabel;

enum FormatFlags {
  FORMAT_QUALITY	= 0x0001,
  FORMAT_STATUS	= 0x0002,	// manual & locked modifiers
  FORMAT_UNITS	= 0x0004,
  FORMAT_COLOR	= 0x0008,

  FORMAT_DEFAULT	= FORMAT_QUALITY | FORMAT_STATUS | FORMAT_UNITS
};

std::string FormatFloat(double val, const char* fmt);

void EscapeColoredString(base::string16& str);

bool StringToValue(base::StringPiece str, const scada::NodeId& data_type_id, scada::Variant& value);
bool StringToValue(base::StringPiece16 str, const scada::NodeId& data_type_id, scada::Variant& value);