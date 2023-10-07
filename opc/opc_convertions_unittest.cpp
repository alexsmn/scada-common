#include "opc/opc_convertions.h"

#include "opc/variant_converter.h"

#include <gmock/gmock.h>
#include <opcda.h>

using namespace testing;

namespace opc {

TEST(OpcQualityConverter, Good) {
  // Good quality.
  EXPECT_EQ(OpcQualityConverter::ToScada(opc_client::Quality::GOOD),
            scada::Qualifier{});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::GOOD_LOCAL_OVERRIDE),
      scada::Qualifier{scada::Qualifier::MANUAL});
}

TEST(OpcQualityConverter, Uncertain) {
  EXPECT_EQ(OpcQualityConverter::ToScada(opc_client::Quality::UNCERTAIN),
            scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::UNCERTAIN_LAST_USABLE),
      scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::UNCERTAIN_SENSOR_CAL),
      scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::UNCERTAIN_EGU_EXCEEDED),
      scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::UNCERTAIN_SUB_NORMAL),
      scada::Qualifier{scada::Qualifier::STALE});
}

TEST(OpcQualityConverter, Bad) {
  EXPECT_EQ(OpcQualityConverter::ToScada(opc_client::Quality::BAD),
            scada::Qualifier{scada::Qualifier::BAD});
  EXPECT_EQ(OpcQualityConverter::ToScada(opc_client::Quality::BAD_CONFIG_ERROR),
            scada::Qualifier{scada::Qualifier::MISCONFIGURED});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::BAD_NOT_CONNECTED),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::BAD_DEVICE_FAILURE),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::BAD_SENSOR_FAILURE),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::ToScada(opc_client::Quality::BAD_LAST_KNOWN),
            scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::ToScada(opc_client::Quality::BAD_COMM_FAILURE),
            scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(
      OpcQualityConverter::ToScada(opc_client::Quality::BAD_OUT_OF_SERVICE),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::ToScada(
                opc_client::Quality::BAD_WAITING_FOR_INITIAL_DATA),
            scada::Qualifier{scada::Qualifier::OFFLINE});
}

TEST(OpcDataValueConverter, ToScada) {
  auto now = scada::DateTime::Now();
  auto timestamp = now - scada::Duration::FromSeconds(123);

  auto opc_data_value =
      opc_client::DataValue{.value = 123, .timestamp = timestamp.ToFileTime()};

  auto scada_data_value = OpcDataValueConverter::ToScada(opc_data_value, now);

  EXPECT_EQ(scada_data_value.value, 123);
  EXPECT_EQ(scada_data_value.qualifier, scada::Qualifier{});
  EXPECT_EQ(scada_data_value.source_timestamp, timestamp);
  EXPECT_EQ(scada_data_value.server_timestamp, now);
  EXPECT_EQ(scada_data_value.status_code, scada::StatusCode::Good);
}

TEST(OpcDataValueConverter, ToOpc) {
  auto server_timestamp = scada::DateTime::Now();
  auto source_timestamp = server_timestamp - scada::Duration::FromSeconds(123);

  auto scada_data_value =
      scada::DataValue{/*value*/ 123,
                       /*quality*/ scada::Qualifier{},
                       /*source_timestamp*/ source_timestamp,
                       /*server_timestamp*/ server_timestamp};

  auto opc_data_value = OpcDataValueConverter::ToOpc(scada_data_value);

  EXPECT_EQ(VariantConverter::ToScada(opc_data_value.value), 123);
  EXPECT_EQ(opc_data_value.quality, opc_client::Quality::GOOD);
  EXPECT_EQ(scada::DateTime::FromFileTime(opc_data_value.timestamp),
            source_timestamp);
  EXPECT_EQ(opc_data_value.status, S_OK);
}

}  // namespace opc