#pragma once

#include "base/strings/string16.h"
#include "common/format.h"

namespace scada {

class Node;
class Variant;

base::string16 FormatValue(const Node& node,
                           const Variant& value,
                           Qualifier qualifier,
                           int flags = FORMAT_DEFAULT);

}  // namespace scada