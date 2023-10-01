#include "opc/variant_converter.h"

#include <format>
#include <gmock/gmock.h>

using namespace testing;

namespace opc {

// GTest requires test parameters to be copyable.
using TestParam =
    std::shared_ptr<const std::pair<scada::Variant, base::win::ScopedVariant>>;

inline TestParam WrapTestParam(scada::Variant scada_variant,
                               base::win::ScopedVariant win_variant) {
  return std::make_shared<
      const std::pair<scada::Variant, base::win::ScopedVariant>>(
      std::move(scada_variant), std::move(win_variant));
}

// `base::win::ScopedVariant` cannot initialize from `float`s.
inline base::win::ScopedVariant MakeFloatWinVariant(float float_value) {
  base::win::ScopedVariant v;
  v.Set(float_value);
  assert(v.type() == VT_R4);
  return v;
}

class VariantConverterTest : public TestWithParam<TestParam> {};

INSTANTIATE_TEST_SUITE_P(
    VariantConverterTests,
    VariantConverterTest,
    Values(WrapTestParam(scada::Variant{}, base::win::ScopedVariant{}),
           WrapTestParam(scada::Variant{static_cast<scada::Int8>(123)},
                         base::win::ScopedVariant{123L, VT_I1}),
           WrapTestParam(scada::Variant{static_cast<scada::UInt8>(123)},
                         base::win::ScopedVariant{123L, VT_UI1}),
           WrapTestParam(scada::Variant{static_cast<scada::Int16>(123)},
                         base::win::ScopedVariant{123L, VT_I2}),
           WrapTestParam(scada::Variant{static_cast<scada::UInt16>(123)},
                         base::win::ScopedVariant{123L, VT_UI2}),
           WrapTestParam(scada::Variant{static_cast<scada::Int32>(123)},
                         base::win::ScopedVariant{123L, VT_I4}),
           WrapTestParam(scada::Variant{static_cast<scada::UInt32>(123)},
                         base::win::ScopedVariant{123L, VT_UI4}),
           WrapTestParam(scada::Variant{static_cast<scada::Int64>(123)},
                         base::win::ScopedVariant{123L, VT_I8}),
           WrapTestParam(scada::Variant{static_cast<scada::UInt64>(123)},
                         base::win::ScopedVariant{123L, VT_UI8}),
           WrapTestParam(scada::Variant{static_cast<scada::Double>(123.0)},
                         MakeFloatWinVariant(123.0F)),
           WrapTestParam(scada::Variant{static_cast<scada::Double>(123.0)},
                         base::win::ScopedVariant{123.0, VT_R8})),
    ([](const TestParamInfo<TestParam>& info) {
      const auto& [scada_variant, win_variant] = *info.param;
      return std::format("{}_and_{}", ToString(scada_variant.type()),
                         ToString(win_variant.type()));
    }));

TEST_P(VariantConverterTest, ConvertForwardAndBackward) {
  const auto& [scada_variant, win_variant] = *GetParam();

  auto forward_result = VariantConverter::Convert(scada_variant);
  ASSERT_NE(forward_result, std::nullopt);

  auto backward_result = VariantConverter::Convert(*forward_result);
  ASSERT_NE(backward_result, std::nullopt);

  ASSERT_EQ(*backward_result, scada_variant);
}

TEST_P(VariantConverterTest, ConvertBackwardAndForward) {
  const auto& [scada_variant, win_variant] = *GetParam();

  auto backward_result = VariantConverter::Convert(win_variant);
  ASSERT_NE(backward_result, std::nullopt);

  auto forward_result = VariantConverter::Convert(*backward_result);
  ASSERT_NE(forward_result, std::nullopt);

  // TODO: Implement comparison for `VARIANT`s.
  // ASSERT_EQ(*backward_result, win_variant);
}

}  // namespace opc