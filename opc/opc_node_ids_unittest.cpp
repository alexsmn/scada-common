#include "opc/opc_node_ids.h"

#include "model/nested_node_ids.h"
#include "model/opc_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace opc {

TEST(ParseOpcNodeId, ServerNode) {
  EXPECT_THAT(
      ParseOpcNodeId(MakeNestedNodeId(id::OPC, "Server.Prog.ID")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Server, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "")));

  EXPECT_THAT(
      ParseOpcNodeId(MakeNestedNodeId(id::OPC, "Server.Prog.ID\\")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Server, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "")));
}

TEST(ParseOpcNodeId, BranchNode) {
  EXPECT_THAT(
      ParseOpcNodeId(MakeNestedNodeId(id::OPC, "Server.Prog.ID\\A.B.C\\")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Branch, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "A.B.C")));
}

TEST(ParseOpcNodeId, ItemNode) {
  EXPECT_THAT(
      ParseOpcNodeId(MakeNestedNodeId(id::OPC, "Server.Prog.ID\\A.B.C")),
      Optional(FieldsAre(/*type*/ OpcAddressType::Item, /*machine_name*/ "",
                         /*prog_id*/ "Server.Prog.ID",
                         /*item_id*/ "A.B.C")));
}

TEST(ParseOpcNodeId, EmptyProgId) {
  EXPECT_EQ(std::nullopt, ParseOpcNodeId(MakeNestedNodeId(id::OPC, "\\A.B.C")));
}

TEST(ParseOpcNodeId, ItemWithMachineName) {
  EXPECT_THAT(ParseOpcNodeId(MakeNestedNodeId(
                  id::OPC, "\\\\localhost\\Server.Prog.ID\\A.B.C")),
              Optional(FieldsAre(/*type*/ OpcAddressType::Item,
                                 /*machine_name*/ "localhost",
                                 /*prog_id*/ "Server.Prog.ID",
                                 /*item_id*/ "A.B.C")));
}

TEST(MakeOpcServerNodeId, Test) {
  EXPECT_EQ(MakeOpcServerNodeId("Server.Prog.ID"),
            MakeNestedNodeId(id::OPC, "Server.Prog.ID"));
}

TEST(MakeOpcBranchNodeId, Test) {
  EXPECT_EQ(MakeOpcBranchNodeId("Server.Prog.ID", "A.B.C"),
            MakeNestedNodeId(id::OPC, "Server.Prog.ID\\A.B.C\\"));
}

TEST(MakeOpcBranchNodeId, EmptyItemId) {
  EXPECT_EQ(MakeOpcBranchNodeId("Server.Prog.ID", /*item_id*/ ""),
            MakeNestedNodeId(id::OPC, "Server.Prog.ID"));
}

TEST(MakeOpcItemNodeId, Test) {
  EXPECT_EQ(MakeOpcItemNodeId("Server.Prog.ID", "A.B.C"),
            MakeNestedNodeId(id::OPC, "Server.Prog.ID\\A.B.C"));
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
