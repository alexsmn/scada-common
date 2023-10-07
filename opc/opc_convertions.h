#pragma once

#include "scada/data_value.h"

#include <Windows.h>
#include <opc_client/core/data_value.h>
#include <optional>

namespace opc {

class OpcQualityConverter {
 public:
  OpcQualityConverter() = delete;

  static scada::Qualifier ToScada(opc_client::Quality quality);
  static opc_client::Quality ToOpc(scada::Qualifier qualifier);
};

class OpcDataValueConverter {
 public:
  OpcDataValueConverter() = delete;

  static scada::DataValue ToScada(const opc_client::DataValue& opc_data_value,
                                  scada::DateTime now);
  static opc_client::DataValue ToOpc(const scada::DataValue& data_value);
};

DATE ToDATE(const FILETIME& file_time);

}  // namespace opc
