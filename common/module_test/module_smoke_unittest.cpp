// Smoke test for the scada.common module facade: names come from
// `import scada.common;` only, including scada.core / scada.base names
// re-exported through `export import`.

#include <string>
#include <vector>

#include <gtest/gtest.h>

// Import after the textual includes (AppleClang 21 libc++ merging bug).
import scada.common;

namespace scada_common_module {
namespace {

TEST(ScadaCommonModuleSmoke, NodeStateAndProperties) {
  scada::NodeProperties properties;
  scada::SetOrAddOrDeleteProperty(properties, scada::NodeId{1, 0},
                                  scada::Variant{scada::Int32{5}});
  const scada::Variant* property =
      scada::FindProperty(properties, scada::NodeId{1, 0});
  ASSERT_NE(property, nullptr);
}

TEST(ScadaCommonModuleSmoke, ExpressionAndFormat) {
  ScadaExpression expression;
  EXPECT_TRUE(expression.ParseStatus("a + b"));
  EXPECT_FALSE(expression.Format().empty());

  // ::Format overload set spans base and common headers.
  EXPECT_EQ(Format(7), "7");
  EXPECT_FALSE(FormatFloat(1.5, "%.2f").empty());

  // Transitive surfaces.
  scada::base::Check(true, "common module smoke");
  EXPECT_TRUE(scada::Status{scada::StatusCode::Good});
}

}  // namespace
}  // namespace scada_common_module
