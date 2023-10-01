#include "opc/opc_convertions.h"

#include <gmock/gmock.h>
#include <opcda.h>

using namespace testing;

namespace opc {

TEST(OpcQualityConverter, Good) {
  // Good quality.
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_GOOD), scada::Qualifier{});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_GOOD |
                                         OPC_QUALITY_LOCAL_OVERRIDE),
            scada::Qualifier{scada::Qualifier::MANUAL});
}

TEST(OpcQualityConverter, Uncertain) {
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_UNCERTAIN),
            scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_UNCERTAIN |
                                         OPC_QUALITY_LAST_USABLE),
            scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_UNCERTAIN |
                                         OPC_QUALITY_SENSOR_CAL),
            scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_UNCERTAIN |
                                         OPC_QUALITY_EGU_EXCEEDED),
            scada::Qualifier{scada::Qualifier::STALE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_UNCERTAIN |
                                         OPC_QUALITY_SUB_NORMAL),
            scada::Qualifier{scada::Qualifier::STALE});
}

TEST(OpcQualityConverter, Bad) {
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_BAD),
            scada::Qualifier{scada::Qualifier::BAD});
  EXPECT_EQ(
      OpcQualityConverter::Convert(OPC_QUALITY_BAD | OPC_QUALITY_CONFIG_ERROR),
      scada::Qualifier{scada::Qualifier::MISCONFIGURED});
  EXPECT_EQ(
      OpcQualityConverter::Convert(OPC_QUALITY_BAD | OPC_QUALITY_NOT_CONNECTED),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_BAD |
                                         OPC_QUALITY_DEVICE_FAILURE),
            scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_BAD |
                                         OPC_QUALITY_SENSOR_FAILURE),
            scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(
      OpcQualityConverter::Convert(OPC_QUALITY_BAD | OPC_QUALITY_LAST_KNOWN),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(
      OpcQualityConverter::Convert(OPC_QUALITY_BAD | OPC_QUALITY_COMM_FAILURE),
      scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_BAD |
                                         OPC_QUALITY_OUT_OF_SERVICE),
            scada::Qualifier{scada::Qualifier::OFFLINE});
  EXPECT_EQ(OpcQualityConverter::Convert(OPC_QUALITY_BAD |
                                         OPC_QUALITY_WAITING_FOR_INITIAL_DATA),
            scada::Qualifier{scada::Qualifier::OFFLINE});
}

}  // namespace opc