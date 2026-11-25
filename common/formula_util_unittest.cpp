#include "common/formula_util.h"

#include "model/namespaces.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>

namespace {

// A connection that gave the plant server's namespace index 200, and handed
// out nothing else past the model's own namespaces
// (docs/product/foreign-node-references.md).
constexpr scada::NamespaceIndex kPlantIndex = 200;

std::optional<std::string> PlantUri(scada::NamespaceIndex index) {
  if (index == kPlantIndex)
    return "urn:plant";
  return std::nullopt;
}

std::optional<scada::NamespaceIndex> PlantIndex(std::string_view uri) {
  if (uri == "urn:plant")
    return kPlantIndex;
  return std::nullopt;
}

TEST(FormulaUtilTest, StoredFormulaNamesAForeignNodeByUri) {
  EXPECT_EQ(MakeStoredNodeIdFormula(
                scada::NodeId{std::string{"Plant.Temperature"}, kPlantIndex},
                PlantUri),
            "{nsu=urn:plant;s=Plant.Temperature}");
  EXPECT_EQ(MakeStoredNodeIdFormula(scada::NodeId{5u, kPlantIndex}, PlantUri),
            "{nsu=urn:plant;i=5}");
}

TEST(FormulaUtilTest, StoredFormulaKeepsTheShortNameForAModelNamespace) {
  const scada::NodeId ts{1379u, scada::NamespaceIndexes::TS};
  EXPECT_EQ(MakeStoredNodeIdFormula(ts, PlantUri), MakeNodeIdFormula(ts));
  EXPECT_EQ(MakeStoredNodeIdFormula(ts, PlantUri), "TS.1379");
}

TEST(FormulaUtilTest, StoredFormulaNeverBracesAClosingBrace) {
  EXPECT_EQ(MakeStoredNodeIdFormula(
                scada::NodeId{std::string{"a}b"}, kPlantIndex}, PlantUri),
            "");
}

TEST(FormulaUtilTest, ReferencesRoundTripThroughStorage) {
  struct Case {
    std::string_view session;
    std::string_view stored;
  };
  const Case kCases[] = {
      // A bare reference, as NodeIdToScadaString writes a window item's path.
      {"NS200.Plant.Temperature", "nsu=urn:plant;s=Plant.Temperature"},
      // MakeNodeIdFormula's forms: numeric unbraced, string braced.
      {"NS200.5", "nsu=urn:plant;i=5"},
      {"{NS200.Plant.Temperature}", "{nsu=urn:plant;s=Plant.Temperature}"},
      // A sheet cell.
      {"={NS200.Plant.Temperature}", "={nsu=urn:plant;s=Plant.Temperature}"},
      // Braced operands of an expression.
      {"{NS200.A} + {NS200.B} * 2",
       "{nsu=urn:plant;s=A} + {nsu=urn:plant;s=B} * 2"},
  };
  for (const Case& c : kCases) {
    SCOPED_TRACE(c.session);
    EXPECT_EQ(FormulaReferencesToStored(c.session, PlantUri), c.stored);
    EXPECT_EQ(FormulaReferencesFromStored(c.stored, PlantIndex), c.session);
  }
}

TEST(FormulaUtilTest, OtherTextIsKeptAsWritten) {
  for (std::string_view text :
       {"TS.1379", "{TS.1379} + 1", "Alias", "{u35_s1}",
        "MODBUS_PORTS.32!Online", "Server!CPU", "", "=", "Voltage", "{unclosed",
        // An index the connection never handed out has no URI to write.
        "{NS300.X}",
        // Our own namespace by URI, typed by hand: readable as it is.
        "{nsu=http://telecontrol.ru/opcua/data_items/DiscreteItemType;i=1}",
        // A URI this connection cannot resolve.
        "{nsu=urn:other;s=X}"}) {
    SCOPED_TRACE(text);
    EXPECT_EQ(FormulaReferencesToStored(text, PlantUri), text);
    EXPECT_EQ(FormulaReferencesFromStored(text, PlantIndex), text);
  }
}

}  // namespace
