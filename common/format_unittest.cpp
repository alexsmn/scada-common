#include "format.h"

#include "scada/variant.h"
#include "scada/variant_utils.h"

#include <gmock/gmock.h>
#include <optional>

namespace {

template <class T>
T StringToValueHelper(std::string_view str) {
  auto data_type = scada::ToBuiltInDataType<T>();

  scada::Variant value;
  if (!StringToValue(str, data_type, value)) {
    assert(value.is_null());
    return std::numeric_limits<T>::min();
  }

  const auto* v = value.get_if<T>();
  return v ? *v : std::numeric_limits<T>::min();
}

}  // namespace

// FormatFloat

TEST(FormatFloat, EmptyFormat) {
  EXPECT_EQ("10", FormatFloat(10.0, ""));
}

TEST(FormatFloat, FixedDecimals) {
  EXPECT_EQ("10.00", FormatFloat(10.0, "0.00"));
  EXPECT_EQ("3.14", FormatFloat(3.14159, "0.00"));
}

TEST(FormatFloat, OptionalDecimalsTrailingZerosStripped) {
  EXPECT_EQ("10", FormatFloat(10.0, "0.####"));
  EXPECT_EQ("10.5", FormatFloat(10.5, "0.####"));
  EXPECT_EQ("10.12", FormatFloat(10.12, "0.####"));
  EXPECT_EQ("10.1235", FormatFloat(10.12345, "0.####"));
}

TEST(FormatFloat, MixedFixedAndOptionalDecimals) {
  // "0.00##" means 2 fixed + 2 optional decimals.
  EXPECT_EQ("10.00", FormatFloat(10.0, "0.00##"));
  EXPECT_EQ("10.50", FormatFloat(10.5, "0.00##"));
  EXPECT_EQ("10.123", FormatFloat(10.123, "0.00##"));
  EXPECT_EQ("10.1235", FormatFloat(10.12345, "0.00##"));
}

TEST(FormatFloat, NoDecimalPart) {
  EXPECT_EQ("10", FormatFloat(10.4, "0"));
}

// StringToValue

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
