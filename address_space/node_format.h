#pragma once

#include "common/format.h"
#include "core/localized_text.h"
#include "core/string.h"

namespace scada {

class Node;
class Variant;

std::wstring FormatDiscreteValue(const LocalizedText& open_label,
                                 const LocalizedText& close_label,
                                 bool locked,
                                 const Variant& value,
                                 Qualifier qualifier,
                                 int flags = FORMAT_DEFAULT);

std::wstring FormatAnalogValue(const String& display_format,
                               const LocalizedText& eu_units,
                               bool locked,
                               const Variant& value,
                               Qualifier qualifier,
                               int flags = FORMAT_DEFAULT);

std::wstring FormatUnknownValue(bool locked,
                                const Variant& value,
                                Qualifier qualifier,
                                int flags = FORMAT_DEFAULT);

std::wstring FormatValue(const Node& node,
                         const Variant& value,
                         Qualifier qualifier,
                         int flags = FORMAT_DEFAULT);

}  // namespace scada
