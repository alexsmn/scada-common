#include "node_service/v1/node_service_impl.h"
#include "node_service/v2/node_service_impl.h"
#include "node_service/v3/node_service_impl.h"

#include "address_space/test/test_address_space.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/test_executor.h"
#include "core/method_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "node_service/mock_node_observer.h"

#include <gmock/gmock.h>

using namespace testing;

namespace v1 {

struct NodeServiceTestContext {
  NodeServiceTestContext(std::shared_ptr<Executor> executor,
                         std::shared_ptr<TestAddressSpace> server_address_space,
                         ViewEventsProvider view_events_provider)
      : executor{std::move(executor)},
        server_address_space{std::move(server_address_space)},
        view_events_provider{std::move(view_events_provider)} {}

  ~NodeServiceTestContext() { client_address_space.Clear(); }

  const std::shared_ptr<Executor> executor;
  const std::shared_ptr<TestAddressSpace> server_address_space;
  const ViewEventsProvider view_events_provider;

  AddressSpaceImpl client_address_space{std::make_shared<NullLogger>()};
  StandardAddressSpace client_standard_address_space{client_address_space};

  GenericNodeFactory node_factory{client_address_space};

  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  NiceMock<scada::MockMethodService> method_service;

  NodeServiceImpl node_service{NodeServiceImplContext{
      executor, view_events_provider, *server_address_space,
      *server_address_space, client_address_space, node_factory,
      monitored_item_service, method_service}};
};

}  // namespace v1

template <class NodeServiceImpl>
class NodeServiceTest : public Test {
 public:
  NodeServiceTest();
  ~NodeServiceTest();

 protected:
  std::shared_ptr<NodeServiceImpl> CreateNodeServiceImpl();
  ViewEventsProvider MakeViewEventsProvider();

  MockNodeObserver node_observer_;

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  const std::shared_ptr<TestAddressSpace> server_address_space_ =
      std::make_shared<TestAddressSpace>();

  scada::ViewEvents* view_events_ = nullptr;
  ViewEventsProvider view_events_provider_ = MakeViewEventsProvider();

  const std::shared_ptr<NodeServiceImpl> node_service_ =
      CreateNodeServiceImpl();
};

using NodeServiceImpls =
    Types<v1::NodeServiceImpl /*, v2::NodeServiceImpl, v3::NodeServiceImpl*/>;
TYPED_TEST_SUITE(NodeServiceTest, NodeServiceImpls);

template <>
NodeServiceTest<v1::NodeServiceImpl>::NodeServiceTest() {
  node_service_->Subscribe(node_observer_);

  // v1's |AddressSpaceFetcher| fetches the whole address space on channel open.
  EXPECT_CALL(this->node_observer_, OnModelChanged(_)).Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeFetched(_, _)).Times(AnyNumber());

  node_service_->OnChannelOpened();

  EXPECT_CALL(this->node_observer_, OnModelChanged(_)).Times(0);
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(_)).Times(0);
  EXPECT_CALL(this->node_observer_, OnNodeFetched(_, _)).Times(0);
}

template <>
NodeServiceTest<v1::NodeServiceImpl>::~NodeServiceTest() {
  node_service_->Unsubscribe(node_observer_);
}

template <>
std::shared_ptr<v1::NodeServiceImpl>
NodeServiceTest<v1::NodeServiceImpl>::CreateNodeServiceImpl() {
  auto context = std::make_shared<v1::NodeServiceTestContext>(
      executor_, server_address_space_, view_events_provider_);
  return std::shared_ptr<v1::NodeServiceImpl>(context, &context->node_service);
}

template <>
ViewEventsProvider
NodeServiceTest<v1::NodeServiceImpl>::MakeViewEventsProvider() {
  return [this](scada::ViewEvents& events)
             -> std::unique_ptr<IViewEventsSubscription> {
    assert(!view_events_);
    view_events_ = &events;
    return std::make_unique<IViewEventsSubscription>();
  };
}

TYPED_TEST(NodeServiceTest, FetchNode) {
  EXPECT_CALL(this->node_observer_, OnModelChanged(_)).Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeFetched(_, _)).Times(AnyNumber());

  auto node =
      this->node_service_->GetNode(this->server_address_space_->kTestNode2Id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "TestNode2");
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp1.Value"},
            node[this->server_address_space_->kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp2.Value"},
            node[this->server_address_space_->kTestProp2Id].value());
}

TYPED_TEST(NodeServiceTest, FetchUnknownNode) {
  const scada::NodeId unknown_node_id{1, 100};

  const scada::ModelChangeEvent reference_added_delete_event{
      unknown_node_id,
      {},
      scada::ModelChangeEvent::ReferenceAdded |
          scada::ModelChangeEvent::ReferenceDeleted};

  EXPECT_CALL(this->node_observer_,
              OnModelChanged(reference_added_delete_event));
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(unknown_node_id));
  EXPECT_CALL(this->node_observer_, OnNodeFetched(unknown_node_id, false));

  auto node = this->node_service_->GetNode(unknown_node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
}

TYPED_TEST(NodeServiceTest, NodeAdded) {
  const scada::NodeState new_node_state{
      scada::NodeId{"NewNodeId", 0},
      scada::NodeClass::Object,
      this->server_address_space_->kTestTypeId,
      scada::id::RootFolder,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_browse_name("NewNode"),
  };

  this->server_address_space_->CreateNode(new_node_state);

  // v1 fetches the node immediately on model change notification reception.
  EXPECT_CALL(this->node_observer_, OnModelChanged(_)).Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeFetched(_, _)).Times(AnyNumber());

  const scada::ModelChangeEvent node_added_event{
      new_node_state.node_id, new_node_state.type_definition_id,
      scada::ModelChangeEvent::NodeAdded};

  EXPECT_CALL(this->node_observer_, OnModelChanged(node_added_event));

  this->view_events_->OnModelChanged(node_added_event);

  Mock::VerifyAndClearExpectations(&this->node_observer_);

  EXPECT_CALL(this->node_observer_, OnModelChanged(_)).Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_observer_, OnNodeFetched(_, _)).Times(AnyNumber());

  auto node = this->node_service_->GetNode(new_node_state.node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "NewNode");
}

TYPED_TEST(NodeServiceTest, NodeDeleted) {
  const scada::NodeId deleted_node_id =
      this->server_address_space_->kTestNode1Id;
  ASSERT_TRUE(this->server_address_space_->GetNode(deleted_node_id));

  const scada::ModelChangeEvent node_deleted_event{
      deleted_node_id, {}, scada::ModelChangeEvent::NodeDeleted};

  EXPECT_CALL(this->node_observer_, OnModelChanged(node_deleted_event));

  this->view_events_->OnModelChanged(node_deleted_event);

  this->server_address_space_->DeleteNode(deleted_node_id);

  Mock::VerifyAndClearExpectations(&this->node_observer_);

  // Now node is unknown.

  const scada::ModelChangeEvent reference_added_delete_event{
      deleted_node_id,
      {},
      scada::ModelChangeEvent::ReferenceAdded |
          scada::ModelChangeEvent::ReferenceDeleted};

  EXPECT_CALL(this->node_observer_,
              OnModelChanged(reference_added_delete_event));
  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(deleted_node_id));
  EXPECT_CALL(this->node_observer_, OnNodeFetched(deleted_node_id, false));

  auto node = this->node_service_->GetNode(deleted_node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
}

TYPED_TEST(NodeServiceTest, NodeSemanticsChanged) {
  const scada::NodeId changed_node_id =
      this->server_address_space_->kTestNode1Id;
  const scada::LocalizedText new_display_name{
      base::WideToUTF16(L"NewTestNode1")};
  const scada::Variant new_property_value{"TestNode1.TestProp1.NewValue"};

  auto node = this->node_service_->GetNode(changed_node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  EXPECT_CALL(this->node_observer_, OnNodeSemanticChanged(changed_node_id))
      .Times(AtLeast(1));

  this->server_address_space_->ModifyNode(
      changed_node_id,
      scada::NodeAttributes{}.set_display_name(new_display_name),
      {{this->server_address_space_->kTestProp1Id, new_property_value}});

  this->view_events_->OnNodeSemanticsChanged(
      scada::SemanticChangeEvent{changed_node_id});

  EXPECT_EQ(new_display_name, node.display_name());
  EXPECT_EQ(new_property_value,
            node[this->server_address_space_->kTestProp1Id].value());
}
