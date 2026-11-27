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

// A data item's formula on the Server names a foreign node by URI
// (docs/product/foreign-node-references.md): braced, unbraced, or as an
// expression's only operand.
TEST(FormulaUtilTest, SingleNodeIdReadsTheNamespaceUriForm) {
  const scada::NodeId expected{std::string{"Plant.Setpoint"}, kPlantIndex};
  EXPECT_EQ(
      GetFormulaSingleNodeId("{nsu=urn:plant;s=Plant.Setpoint}", PlantIndex),
      expected);
  EXPECT_EQ(GetFormulaSingleNodeId("{nsu=urn:plant;i=5}", PlantIndex),
            (scada::NodeId{5u, kPlantIndex}));
}

// Our own namespace by URI resolves through the model's canonical table even
// when the lookup does not know it.
TEST(FormulaUtilTest, SingleNodeIdReadsAModelNamespaceUriWithoutALookup) {
  constexpr std::string_view kName =
      "nsu=http://telecontrol.ru/opcua/data_items/DiscreteItemType;i=1";
  const std::optional<scada::NodeId> expected =
      scada::NodeIdFromNodeIdString(kName, {});
  ASSERT_TRUE(expected);
  ASSERT_FALSE(expected->is_null());
  EXPECT_EQ(
      GetFormulaSingleNodeId("{nsu=http://telecontrol.ru/opcua/data_items/"
                             "DiscreteItemType;i=1}",
                             {}),
      *expected);
}

// An unknown URI names nothing: the resolver then falls back to the alias
// table, which holds no such name either.
TEST(FormulaUtilTest, SingleNodeIdOfAnUnknownUriIsNull) {
  EXPECT_TRUE(
      GetFormulaSingleNodeId("{nsu=urn:other;s=X}", PlantIndex).is_null());
  EXPECT_TRUE(GetFormulaSingleNodeId("{nsu=urn:plant;s=X}", {}).is_null());
}

// Every form the one-argument overload reads, it reads the same with a lookup.
TEST(FormulaUtilTest, SingleNodeIdKeepsEveryOtherForm) {
  for (std::string_view formula :
       {"TS.1379", "{TS.1379}", "NS200.5", "{NS200.Plant.Temperature}", "Alias",
        "{u35_s1}", "Server!CPU", "MODBUS_PORTS.32!Online", "", "{TS.1379} + 1",
        "{i=2258}", "{ns=200;s=X}"}) {
    SCOPED_TRACE(formula);
    EXPECT_EQ(GetFormulaSingleNodeId(formula, PlantIndex),
              GetFormulaSingleNodeId(formula));
  }
}

}  // namespace
