#include "common/scada_expression.h"

#include <gmock/gmock.h>

TEST(ScadaExpression, IsSingleName) {
  std::string name;
  EXPECT_TRUE(ScadaExpression::IsSingleName("{%^@sdg#$%#DgaAS$#@}", name));
}

TEST(ScadaExpression, Function) {
  ScadaExpression expression;
  expression.Parse("or(TIT.1, TIT.2)");
  EXPECT_EQ(2, expression.items.size());
  EXPECT_EQ("TIT.1", expression.items[0].name);
  EXPECT_EQ("TIT.2", expression.items[1].name);
  expression.items[0].value = scada::DataValue{1, {}, {}, {}};
  expression.items[1].value = scada::DataValue{1, {}, {}, {}};
  EXPECT_EQ(scada::Variant{1.0}, expression.Calculate());
}
