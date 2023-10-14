#include "vidicon/data_point_address.h"

#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

TEST(DataPointAddress, ParseOpcDaAddress) {
  EXPECT_EQ(
      ParseDataPointAddress(LR"(VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)"),
      DataPointAddress{.opc_address =
                           LR"(VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)"});
}

TEST(DataPointAddress, ParseOpcAeAddress) {
  EXPECT_EQ(ParseDataPointAddress(
                LR"(AE:VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)"),
            std::nullopt);
}

TEST(DataPointAddress, ParseVidiconAddress) {
  EXPECT_EQ(ParseDataPointAddress(L"CF:456"),
            DataPointAddress{.object_id = 456});
}

}  // namespace vidicon