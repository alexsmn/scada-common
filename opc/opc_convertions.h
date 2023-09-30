#pragma once

#include "opc/opc_types.h"
#include "scada/data_value.h"

#include <opcda.h>
#include <optional>

namespace opc {

class OpcQualityConverter {
 public:
  OpcQualityConverter() = delete;

  static scada::Qualifier Convert(unsigned quality);
  static unsigned Convert(scada::Qualifier qualifier);
};

class OpcDataValueConverter {
 public:
  OpcDataValueConverter() = delete;

  static scada::DataValue Convert(const OpcDataValue& opc_data_value);
  static OpcDataValue Convert(const scada::DataValue& data_value);
};

DATE ToDATE(const FILETIME& file_time);

}  // namespace opc
