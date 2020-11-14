#pragma once

#include "common/format.h"

#include <string>

namespace scada {

class Node;
class Variant;

std::wstring FormatValue(const Node& node,
                         const Variant& value,
                         Qualifier qualifier,
                         int flags = FORMAT_DEFAULT);

}  // namespace scada