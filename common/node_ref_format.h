#pragma once

#include "common/format.h"
#include "common/node_ref.h"

base::string16 FormatValue(const NodeRef& node, const scada::Variant& value, scada::Qualifier qualifier, int flags = FORMAT_DEFAULT);
