#include "format.h"

#include "scada/standard_node_ids.h"
#include "scada/variant.h"
#include "scada/variant_utils.h"

#include <gmock/gmock.h>
#include <optional>

namespace {

template <class T>
T StringToValueHelper(std::string_view str) {
  const scada::NodeId data_type_id = scada::ToBuiltInDataType<T>();

  scada::Variant value;
  if (!StringToValue(str, data_type_id, value)) {
    assert(value.is_null());
    return std::numeric_limits<T>::min();
  }

  const auto* v = value.get_if<T>();
  return v ? *v : std::numeric_limits<T>::min();
}

}  // namespace

TEST(StringToValue, Test) {
  EXPECT_EQ(StringToValueHelper<scada::Int8>("-111"), -111);
  EXPECT_EQ(StringToValueHelper<scada::Int8>("111"), 111);
  EXPECT_EQ(StringToValueHelper<scada::UInt8>("111"), 111U);

  EXPECT_EQ(StringToValueHelper<scada::Int16>("-111"), -111);
  EXPECT_EQ(StringToValueHelper<scada::Int16>("111"), 111);
  EXPECT_EQ(StringToValueHelper<scada::UInt16>("111"), 111U);

  EXPECT_EQ(StringToValueHelper<scada::Int32>("-111"), -111);
  EXPECT_EQ(StringToValueHelper<scada::Int32>("111"), 111);
  EXPECT_EQ(StringToValueHelper<scada::UInt32>("111"), 111U);

  EXPECT_EQ(StringToValueHelper<scada::Int64>("-111"), -111);
  EXPECT_EQ(StringToValueHelper<scada::Int64>("111"), 111);
  EXPECT_EQ(StringToValueHelper<scada::UInt64>("111"), 111U);

  EXPECT_EQ(StringToValueHelper<scada::Double>("-111.222"), -111.222);
  EXPECT_EQ(StringToValueHelper<scada::Double>("111.222"), 111.222);
}
