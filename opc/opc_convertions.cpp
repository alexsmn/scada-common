#include "opc/opc_convertions.h"

#include "base/strings/string_util.h"
#include "opc/variant_converter.h"

#include <boost/locale/encoding_utf.hpp>
#include <opc_client/core/conversion.h>

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
scada::Qualifier OpcQualityConverter::Convert(unsigned quality) {
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
unsigned OpcQualityConverter::Convert(scada::Qualifier qualifier) {
  return qualifier.general_bad() ? OPC_QUALITY_BAD : OPC_QUALITY_GOOD;
}

// OpcDataValueConverter

// static
scada::DataValue OpcDataValueConverter::Convert(
    const OpcDataValue& opc_data_value) {
  auto timestamp = ToDateTime(opc_data_value.timestamp);
  if (auto value = VariantConverter::Convert(opc_data_value.value)) {
    return {std::move(*value),
            OpcQualityConverter::Convert(opc_data_value.quality), timestamp,
            timestamp};
  } else {
    return {scada::Variant{}, scada::Qualifier{scada::Qualifier::BAD},
            timestamp, timestamp};
  }
}

// static
OpcDataValue OpcDataValueConverter::Convert(
    const scada::DataValue& data_value) {
  auto timestamp = data_value.source_timestamp.ToFileTime();
  if (auto value = VariantConverter::Convert(data_value.value)) {
    return {.value = std::move(*value),
            .timestamp = timestamp,
            .quality = OpcQualityConverter::Convert(data_value.qualifier)};
  } else {
    return {.timestamp = timestamp, .quality = OPC_QUALITY_BAD};
  }
}

DATE ToDATE(const FILETIME& file_time) {
  return opc_client::ToDATE(file_time);
}

}  // namespace opc
