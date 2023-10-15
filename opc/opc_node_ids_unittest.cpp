#include "opc/opc_node_ids.h"

#include "model/namespaces.h"
#include "model/nested_node_ids.h"
#include "model/opc_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace opc {

TEST(ParseOpcNodeId, ServerNode) {
  EXPECT_THAT(
      ParseOpcNodeId(MakeOpcServerNodeId("Server.Prog.ID")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Server, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "")));
}

TEST(ParseOpcNodeId, BranchNode) {
  EXPECT_THAT(
      ParseOpcNodeId(MakeOpcBranchNodeId("Server.Prog.ID", "A.B.C")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Branch, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "A.B.C")));
}

TEST(ParseOpcNodeId, ItemNode) {
  EXPECT_THAT(
      ParseOpcNodeId(MakeOpcItemNodeId("Server.Prog.ID", "A.B.C")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Item, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "A.B.C")));
}

TEST(ParseOpcNodeId, EmptyProgId) {
  EXPECT_EQ(std::nullopt,
            ParseOpcNodeId(scada::NodeId{"\\A.B.C", NamespaceIndexes::OPC}));
}

TEST(ParseOpcNodeId, ItemWithMachineName) {
  EXPECT_THAT(
      ParseOpcNodeId(scada::NodeId{"\\\\localhost\\Server.Prog.ID\\A.B.C",
                                   NamespaceIndexes::OPC}),
      Optional(FieldsAre(/*type*/ OpcAddressType::Item,
                         /*machine_name*/ "localhost",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "A.B.C")));
}

TEST(ParseOpcNodeId, NestedNodeId) {
  EXPECT_EQ(ParseOpcNodeId(MakeNestedNodeId(
                MakeOpcItemNodeId("Server.Prog.ID", "A.B.C"), "Aggregate")),
            std::nullopt);
}

TEST(MakeOpcBranchNodeId, EmptyItemId) {
  EXPECT_EQ(MakeOpcBranchNodeId("Server.Prog.ID", /*item_id*/ ""),
            MakeOpcServerNodeId("Server.Prog.ID"));
}

TEST(OpcNodeIds, GetOpcItemName) {
  EXPECT_EQ(GetOpcItemName("RootBranch"), "RootBranch");
  EXPECT_EQ(GetOpcItemName("RootBranch.ChildBranch"), "ChildBranch");
  EXPECT_EQ(GetOpcItemName("RootBranch.ChildBranch.SubChildBranch"),
            "SubChildBranch");
}

TEST(OpcNodeIds, GetOpcCustomParentItemId) {
  EXPECT_EQ(GetOpcCustomParentItemId("RootBranch"), "");
  EXPECT_EQ(GetOpcCustomParentItemId("RootBranch.ChildBranch"), "RootBranch");
  EXPECT_EQ(GetOpcCustomParentItemId("RootBranch.ChildBranch.SubChildBranch"),
            "RootBranch.ChildBranch");
}

}  // namespace opc
