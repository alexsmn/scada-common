#include "address_space/local_method_service.h"
#include "address_space/local_node_management_service.h"

#include "base/test/awaitable_test.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>

namespace scada {
namespace {

TEST(LocalMethodService, CoroutineCallReturnsBadStatus) {
  const auto executor = std::make_shared<TestExecutor>();
  LocalMethodService service;

  const auto status = WaitAwaitable(
      executor,
      service.Call(id::ObjectsFolder, NodeId{1, 2}, {}, NodeId{}));

  EXPECT_EQ(status.code(), StatusCode::Bad);
}

TEST(LocalNodeManagementService, CoroutineAddNodesReturnsBadResults) {
  const auto executor = std::make_shared<TestExecutor>();
  LocalNodeManagementService service;

  auto [status, results] =
      WaitAwaitable(executor,
                    service.AddNodes({AddNodesItem{
                        .requested_id = NodeId{1, 2},
                        .parent_id = id::ObjectsFolder,
                        .node_class = NodeClass::Object,
                        .type_definition_id = id::BaseObjectType}}));

  EXPECT_EQ(status.code(), StatusCode::Bad);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0].status_code, StatusCode::Bad);
}

TEST(LocalNodeManagementService, CoroutineDeleteNodesReturnsBadResults) {
  const auto executor = std::make_shared<TestExecutor>();
  LocalNodeManagementService service;

  auto [status, results] = WaitAwaitable(
      executor,
      service.DeleteNodes({DeleteNodesItem{.node_id = NodeId{1, 2}}}));

  EXPECT_EQ(status.code(), StatusCode::Bad);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0], StatusCode::Bad);
}

TEST(LocalNodeManagementService, CoroutineAddReferencesReturnsBadResults) {
  const auto executor = std::make_shared<TestExecutor>();
  LocalNodeManagementService service;

  auto [status, results] =
      WaitAwaitable(executor,
                    service.AddReferences({AddReferencesItem{
                        .source_node_id = id::ObjectsFolder,
                        .reference_type_id = id::Organizes,
                        .target_node_id = NodeId{1, 2}}}));

  EXPECT_EQ(status.code(), StatusCode::Bad);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0], StatusCode::Bad);
}

TEST(LocalNodeManagementService, CoroutineDeleteReferencesReturnsBadResults) {
  const auto executor = std::make_shared<TestExecutor>();
  LocalNodeManagementService service;

  auto [status, results] =
      WaitAwaitable(executor,
                    service.DeleteReferences({DeleteReferencesItem{
                        .source_node_id = id::ObjectsFolder,
                        .reference_type_id = id::Organizes,
                        .target_node_id = NodeId{1, 2}}}));

  EXPECT_EQ(status.code(), StatusCode::Bad);
  ASSERT_EQ(results.size(), 1u);
  EXPECT_EQ(results[0], StatusCode::Bad);
}

}  // namespace
}  // namespace scada
