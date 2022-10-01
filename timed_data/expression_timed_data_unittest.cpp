#include "timed_data/expression_timed_data.h"

#include "common/scada_expression.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

class TestTimedData : public BaseTimedData {
 public:
  using BaseTimedData::UpdateCurrent;

  // BaseTimedData
  virtual std::string GetFormula(bool aliases) const { return "x"; }
  virtual scada::LocalizedText GetTitle() const override { return u"x"; }
};

}  // namespace

TEST(ExpressionTimedData, Test) {
  auto time = scada::DateTime::Now();

  auto expression = std::make_unique<ScadaExpression>();
  expression->Parse("x + 5");

  auto operand = std::make_shared<TestTimedData>();
  ASSERT_TRUE(operand->UpdateCurrent(scada::DataValue{10, {}, time, time}));

  auto timed_data = std::make_shared<ExpressionTimedData>(
      std::move(expression), std::vector<std::shared_ptr<TimedData>>{operand});

  EXPECT_THAT(
      timed_data->GetDataValue(),
      FieldsAre(15, scada::Qualifier{}, time, _, scada::StatusCode::Good));
}
