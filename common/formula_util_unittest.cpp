#include "common/formula_util.h"

#include "model/namespaces.h"

#include <gmock/gmock.h>

TEST(FormulaUtil, NodeIdFromFormula) {
  EXPECT_EQ(scada::NodeId(5, NamespaceIndexes::TS),
            NodeIdFromFormula("{TS.5}"));
  EXPECT_EQ(scada::NodeId("4!Online", NamespaceIndexes::IEC60870_DEVICE),
            NodeIdFromFormula("{IEC_DEV.4!Online}"));
}
