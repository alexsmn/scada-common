#include "opc/opc_convertions.h"

#include "base/strings/string_util.h"
#include "opc/variant_converter.h"

#include <boost/locale/encoding_utf.hpp>
#include <opc_client/core/conversion.h>
#include <opcda.h>

namespace opc {

namespace {

inline scada::DateTime ToDateTime(FILETIME timestamp) {
  return scada::DateTime::FromFileTime(timestamp);
}

inline std::string ToString(std::wstring_view str) {
  return boost::locale::conv::utf_to_utf<char>(str.data(),
                                               str.data() + str.size());
}

inline scada::Qualifier ConvertBadQuality(unsigned quality) {
  switch (quality) {
    case OPC_QUALITY_CONFIG_ERROR:
      return scada::Qualifier{scada::Qualifier::MISCONFIGURED};
    case OPC_QUALITY_NOT_CONNECTED:
    case OPC_QUALITY_DEVICE_FAILURE:
    case OPC_QUALITY_SENSOR_FAILURE:
    case OPC_QUALITY_LAST_KNOWN:
    case OPC_QUALITY_COMM_FAILURE:
    case OPC_QUALITY_OUT_OF_SERVICE:
    case OPC_QUALITY_WAITING_FOR_INITIAL_DATA:
      return scada::Qualifier{scada::Qualifier::OFFLINE};
    default:
      return scada::Qualifier{scada::Qualifier::BAD};
  }
}

}  // namespace

// OpcQualityConverter

// static
scada::Qualifier OpcQualityConverter::ToScada(unsigned quality) {
  switch (quality & OPC_QUALITY_MASK) {
    case OPC_QUALITY_GOOD:
      return (quality == OPC_QUALITY_LOCAL_OVERRIDE)
                 ? scada::Qualifier{scada::Qualifier::MANUAL}
                 : scada::Qualifier{};
    case OPC_QUALITY_UNCERTAIN:
      return scada::Qualifier{scada::Qualifier::STALE};
    case OPC_QUALITY_BAD:
      return ConvertBadQuality(quality);
    default:
      return scada::Qualifier{scada::Qualifier::BAD};
  }
}

// static
unsigned OpcQualityConverter::ToOpc(scada::Qualifier qualifier) {
  return qualifier.general_bad() ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;
}

// OpcDataValueConverter

// static
scada::DataValue OpcDataValueConverter::ToScada(
    const OpcDataValue& opc_data_value,
    scada::DateTime now) {
  // OPC UA. Part 8. A.3.2.4 Timestamp
  // - The `Timestamp` provided for a value in the DA server is assigned to the
  // `SourceTimeStamp` of the `DataValue` in the COM UA Wrapper.
  //  - The `ServerTimeStamp` in the `DataValue` is set to the current time by
  //  the COM UA Wrapper at the start of the Read Operation.
  auto server_timestamp = now;
  auto source_timestamp = ToDateTime(opc_data_value.timestamp);
  if (auto value = VariantConverter::ToScada(opc_data_value.value)) {
    return {std::move(*value),
            OpcQualityConverter::ToScada(opc_data_value.quality),
            source_timestamp, server_timestamp};
  } else {
    return {scada::Variant{}, scada::Qualifier{scada::Qualifier::BAD},
            source_timestamp, server_timestamp};
  }
}

// static
OpcDataValue OpcDataValueConverter::ToOpc(const scada::DataValue& data_value) {
  // OPC UA. Part 8. A.4.3.4 Timestamp
  // - If available, the `SourceTimestamp` of the `DataValue` in the OPC UA
  // Server is assigned to the `Timestamp` for the value in the COM UA Proxy.
  // - If `SourceTimestamp` is not available, then the `ServerTimestamp` is
  // used.
  auto timestamp = !data_value.source_timestamp.is_null()
                       ? data_value.source_timestamp.ToFileTime()
                       : data_value.server_timestamp.ToFileTime();
  if (auto win_variant_value = VariantConverter::ToWin(data_value.value)) {
    return {.value = std::move(*win_variant_value),
            .timestamp = timestamp,
            .quality = OpcQualityConverter::ToOpc(data_value.qualifier)};
  } else {
    return {.timestamp = timestamp, .quality = OPC_QUALITY_BAD};
  }
}

DATE ToDATE(const FILETIME& file_time) {
  return opc_client::ToDATE(file_time);
}

}  // namespace opc
