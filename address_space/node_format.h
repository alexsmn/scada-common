#pragma once

#include "common/format.h"
#include "scada/localized_text.h"
#include "scada/string.h"

namespace scada {

class Node;
class Variant;

std::u16string FormatDiscreteValue(const LocalizedText& open_label,
                                   const LocalizedText& close_label,
                                   bool locked,
                                   const Variant& value,
                                   Qualifier qualifier,
                                   int flags = FORMAT_DEFAULT);

std::u16string FormatAnalogValue(const String& display_format,
                                 const LocalizedText& eu_units,
                                 bool locked,
                                 const Variant& value,
                                 Qualifier qualifier,
                                 int flags = FORMAT_DEFAULT);

std::u16string FormatUnknownValue(bool locked,
                                  const Variant& value,
                                  Qualifier qualifier,
                                  int flags = FORMAT_DEFAULT);

std::u16string FormatValue(const Node& node,
                           const Variant& value,
                           Qualifier qualifier,
                           int flags = FORMAT_DEFAULT);

}  // namespace scada
