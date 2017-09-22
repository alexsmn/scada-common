#pragma once

#include "core/standard_node_ids.h"
#include "core/view_service.h"

namespace test {

class TestViewService : public scada::ViewService {
 public:
  // scada::ViewService
  virtual void Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) override;
  virtual void TranslateBrowsePath(const scada::NodeId& starting_node_id, const scada::RelativePath& relative_path,
      const scada::TranslateBrowsePathCallback& callback);
  virtual void Subscribe(scada::ViewEvents& events) override {}
  virtual void Unsubscribe(scada::ViewEvents& events) override {}

  const scada::NodeId kHasTypeDefinitionRefTypeId{"kHasTypeDefinitionRefTypeId", 0};
  const scada::NodeId kHasParentRefTypeId{"HasParentRefTypeId", 0};

  const scada::NodeId kTestTypeId{"TestTypeId", 0};
  const scada::NodeId kTestProp1Id{"TestProp1Id", 0};
  const scada::NodeId kTestProp2Id{"TestProp2Id", 0};
  const scada::NodeId kTestRefTypeId{"TestRefTypeId", 0};

  const scada::BrowseNode kRootNode{
      {}, // parent id
      {}, // reference_type_id
      OpcUaId_RootFolder,
      scada::NodeClass::Object,
      OpcUaId_FolderType, // type id
      "Root"
  };

  const scada::BrowseNode kTestNode1{
      kRootNode.node_id, // parent id
      kHasParentRefTypeId, // reference type id
      scada::NodeId{"TestNode1Id", 0},
      scada::NodeClass::Object,
      kTestTypeId, // type id
      "TestNode1",
  };

  const scada::BrowseNode kTestNode2{
      kRootNode.node_id, // parent id
      kHasParentRefTypeId, // reference type id
      scada::NodeId{"TestNode2Id", 0},
      scada::NodeClass::Object,
      kTestTypeId, // type id
      "TestNode2",
  };

  const std::vector<scada::BrowseNode> kNodes{
      kRootNode,
      kTestNode1,
      kTestNode2,
  };

  const std::vector<scada::BrowseReference> kReferences{
      {kTestRefTypeId, kTestNode1.node_id, kTestNode2.node_id},
  };
};

inline void TestViewService::Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) {
  assert(false);
}

void TestViewService::TranslateBrowsePath(const scada::NodeId& starting_node_id, const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  callback(scada::StatusCode::Bad, {}, 0);
}

} // namespace test
