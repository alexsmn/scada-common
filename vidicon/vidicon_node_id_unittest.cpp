#include "vidicon/vidicon_node_id.h"

#include "opc/opc_node_ids.h"

#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

TEST(VidiconNodeId, ToNodeId) {
  EXPECT_EQ(ToNodeId(L"Server.Prog.Id\\A.B.C"),
            opc::MakeOpcItemNodeId("Server.Prog.Id", "A.B.C"));
}

}  // namespace vidicon