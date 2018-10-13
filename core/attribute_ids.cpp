#include "core/attribute_ids.h"

#include "base/strings/string_number_conversions.h"

std::string ToString(scada::AttributeId attribute_id) {
  // TODO:
  return base::NumberToString(static_cast<int>(attribute_id));
}
