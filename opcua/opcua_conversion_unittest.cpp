#include "opcua/opcua_conversion.h"

#include <gmock/gmock.h>

using namespace testing;

TEST(OpcUaConversion, Convert_DateTime) {
  {
    scada::DateTime date_time;
    auto opcua_date_time = Convert(date_time);
    auto restored_date_time = Convert(opcua_date_time);
    EXPECT_EQ(date_time, restored_date_time);
  }

  {
    scada::DateTime date_time = scada::DateTime::Now();
    auto opcua_date_time = Convert(date_time);
    auto restored_date_time = Convert(opcua_date_time);
    EXPECT_EQ(date_time, restored_date_time);
  }
}
