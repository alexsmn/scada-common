#pragma once

#include "opc/opc_types.h"
#include "scada/data_value.h"

#include <Windows.h>
#include <optional>

namespace opc {

class OpcQualityConverter {
 public:
  OpcQualityConverter() = delete;

  static scada::Qualifier ToScada(unsigned quality);
  static unsigned ToOpc(scada::Qualifier qualifier);
};

class OpcDataValueConverter {
 public:
  OpcDataValueConverter() = delete;

  static scada::DataValue ToScada(const OpcDataValue& opc_data_value,
                                  scada::DateTime now);
  static OpcDataValue ToOpc(const scada::DataValue& data_value);
};

DATE ToDATE(const FILETIME& file_time);

}  // namespace opc
