#include "common/scada_expression.h"

#include <gmock/gmock.h>

TEST(ScadaExpression, IsSingleName) {
  std::string name;
  EXPECT_TRUE(ScadaExpression::IsSingleName("{%^@sdg#$%#DgaAS$#@}", name));
}
