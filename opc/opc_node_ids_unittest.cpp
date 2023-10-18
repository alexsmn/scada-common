#include "opc/opc_node_ids.h"

#include "model/namespaces.h"
#include "model/nested_node_ids.h"
#include "model/opc_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace opc {

TEST(ParseOpcNodeId, ServerNode) {
  EXPECT_THAT(ParseOpcNodeId(MakeOpcServerNodeId(L"Server.Prog.ID")),
              Optional(FieldsAre(/*type*/ opc_client::AddressType::Server,
                                 /*machine_name*/ L"",
                                 /*prog_id*/ L"Server.Prog.ID",
                                 /*item_id*/ L"")));
}

TEST(ParseOpcNodeId, BranchNode) {
  EXPECT_THAT(ParseOpcNodeId(MakeOpcBranchNodeId(L"Server.Prog.ID", L"A.B.C")),
              Optional(FieldsAre(/*type*/ opc_client::AddressType::Branch,
                                 /*machine_name*/ L"",
                                 /*prog_id*/ L"Server.Prog.ID",
                                 /*item_id*/ L"A.B.C")));
}

TEST(ParseOpcNodeId, ItemNode) {
  EXPECT_THAT(ParseOpcNodeId(MakeOpcItemNodeId(L"Server.Prog.ID", L"A.B.C")),
              Optional(FieldsAre(/*type*/ opc_client::AddressType::Item,
                                 /*machine_name*/ L"",
                                 /*prog_id*/ L"Server.Prog.ID",
                                 /*item_id*/ L"A.B.C")));
}

TEST(ParseOpcNodeId, EmptyProgId) {
  EXPECT_EQ(std::nullopt,
            ParseOpcNodeId(scada::NodeId{"\\A.B.C", NamespaceIndexes::OPC}));
}

TEST(ParseOpcNodeId, ItemWithMachineName) {
  EXPECT_THAT(
      ParseOpcNodeId(scada::NodeId{"\\\\localhost\\Server.Prog.ID\\A.B.C",
                                   NamespaceIndexes::OPC}),
      Optional(FieldsAre(/*type*/ opc_client::AddressType::Item,
                         /*machine_name*/ L"localhost",
                         /*prog_id*/ L"Server.Prog.ID",
                         /*item_id*/ L"A.B.C")));
}

TEST(ParseOpcNodeId, NestedNodeId) {
  EXPECT_EQ(ParseOpcNodeId(MakeNestedNodeId(
                MakeOpcItemNodeId(L"Server.Prog.ID", L"A.B.C"), "Aggregate")),
            std::nullopt);
}

TEST(MakeOpcBranchNodeId, EmptyItemId) {
  EXPECT_EQ(MakeOpcBranchNodeId(L"Server.Prog.ID", /*item_id*/ L""),
            MakeOpcServerNodeId(L"Server.Prog.ID"));
}

TEST(OpcNodeIds, GetOpcItemName) {
  EXPECT_EQ(GetOpcItemName(L"RootBranch"), L"RootBranch");
  EXPECT_EQ(GetOpcItemName(L"RootBranch.ChildBranch"), L"ChildBranch");
  EXPECT_EQ(GetOpcItemName(L"RootBranch.ChildBranch.SubChildBranch"),
            L"SubChildBranch");
}

TEST(OpcNodeIds, GetOpcCustomParentItemId) {
  EXPECT_EQ(GetOpcCustomParentItemId(L"RootBranch"), L"");
  EXPECT_EQ(GetOpcCustomParentItemId(L"RootBranch.ChildBranch"), L"RootBranch");
  EXPECT_EQ(GetOpcCustomParentItemId(L"RootBranch.ChildBranch.SubChildBranch"),
            L"RootBranch.ChildBranch");
}

}  // namespace opc
