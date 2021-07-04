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

namespace v2 {

struct NodeServiceTestContext {
  NodeServiceTestContext(std::shared_ptr<Executor> executor,
                         std::shared_ptr<TestAddressSpace> server_address_space,
                         ViewEventsProvider view_events_provider)
      : executor{std::move(executor)},
        server_address_space{std::move(server_address_space)},
        view_events_provider{std::move(view_events_provider)} {}

  const std::shared_ptr<Executor> executor;
  const std::shared_ptr<TestAddressSpace> server_address_space;
  const ViewEventsProvider view_events_provider;

  NiceMock<scada::MockMonitoredItemService> monitored_item_service;

  NodeServiceImpl node_service{NodeServiceImplContext{
      executor, *server_address_space, *server_address_space,
      monitored_item_service, view_events_provider}};
};

}  // namespace v2

template <class NodeServiceImpl>
class NodeServiceTest : public Test {
 public:
  NodeServiceTest();
  ~NodeServiceTest();

 protected:
  std::shared_ptr<NodeServiceImpl> CreateNodeServiceImpl();
  ViewEventsProvider MakeViewEventsProvider();

  void OpenChannel();

  void ValidateFetchUnknownNode(const scada::NodeId& unknown_node_id);
  void ValidateNodeFetched(const NodeRef& node);

  StrictMock<MockNodeObserver> node_service_observer_;

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
    Types<v1::NodeServiceImpl, v2::NodeServiceImpl /*, v3::NodeServiceImpl*/>;
TYPED_TEST_SUITE(NodeServiceTest, NodeServiceImpls);

MATCHER_P(NodeIs, node_id, "") {
  return arg.node_id == node_id;
}

MATCHER_P(NodeIsOrIsNestedOf, node_id, "") {
  if (arg.node_id == node_id)
    return true;

  scada::NodeId parent_id;
  std::string_view nested_name;
  return IsNestedNodeId(arg.node_id, parent_id, nested_name) &&
         parent_id == node_id;
}

template <class NodeServiceImpl>
NodeServiceTest<NodeServiceImpl>::NodeServiceTest() {
  node_service_->Subscribe(node_service_observer_);
}

template <class NodeServiceImpl>
NodeServiceTest<NodeServiceImpl>::~NodeServiceTest() {
  node_service_->Unsubscribe(node_service_observer_);
}

template <>
std::shared_ptr<v1::NodeServiceImpl>
NodeServiceTest<v1::NodeServiceImpl>::CreateNodeServiceImpl() {
  auto context = std::make_shared<v1::NodeServiceTestContext>(
      executor_, server_address_space_, view_events_provider_);
  return std::shared_ptr<v1::NodeServiceImpl>(context, &context->node_service);
}

template <>
std::shared_ptr<v2::NodeServiceImpl>
NodeServiceTest<v2::NodeServiceImpl>::CreateNodeServiceImpl() {
  auto context = std::make_shared<v2::NodeServiceTestContext>(
      executor_, server_address_space_, view_events_provider_);
  return std::shared_ptr<v2::NodeServiceImpl>(context, &context->node_service);
}

template <class NodeServiceImpl>
ViewEventsProvider NodeServiceTest<NodeServiceImpl>::MakeViewEventsProvider() {
  return [this](scada::ViewEvents& events)
             -> std::unique_ptr<IViewEventsSubscription> {
    assert(!view_events_);
    view_events_ = &events;
    return std::make_unique<IViewEventsSubscription>();
  };
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::OpenChannel() {
  // v1's |AddressSpaceFetcher| fetches the whole address space on channel open.
  EXPECT_CALL(*this->server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(*this->server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _))
      .Times(AnyNumber());

  node_service_->OnChannelOpened();

  EXPECT_CALL(*this->server_address_space_, Read(_, _)).Times(0);
  EXPECT_CALL(*this->server_address_space_, Browse(_, _)).Times(0);
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_)).Times(0);
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(_)).Times(0);
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _)).Times(0);
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ValidateNodeFetched(
    const NodeRef& node) {
  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());

  // Type definition.
  auto type_definition = node.type_definition();
  EXPECT_TRUE(type_definition);
  EXPECT_TRUE(type_definition.fetched());
  EXPECT_EQ(type_definition.node_id(),
            this->server_address_space_->kTestTypeId);

  // Supertype.
  auto supertype = type_definition.supertype();
  EXPECT_TRUE(supertype);
  EXPECT_TRUE(supertype.fetched());
  EXPECT_EQ(supertype.node_id(), scada::id::BaseObjectType);

  // Attributes.
  EXPECT_EQ(node.node_class(), scada::NodeClass::Object);
  EXPECT_EQ(node.browse_name(), "TestNode2");
  EXPECT_EQ(node.display_name(), L"TestNode2DisplayName");

  // Properties.
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp1.Value"},
            node[this->server_address_space_->kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp2.Value"},
            node[this->server_address_space_->kTestProp2Id].value());
}

TYPED_TEST(NodeServiceTest, FetchNode_BeforeChannelOpen) {
  const auto node_id = this->server_address_space_->kTestNode2Id;
  const auto type_definition_id = this->server_address_space_->kTestTypeId;

  auto node = this->node_service_->GetNode(node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  MockFunction<void(const NodeRef& node)> fetch_callback;
  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  EXPECT_CALL(fetch_callback, Call(_));

  EXPECT_CALL(node_observer, OnNodeFetched(node_id, false));
  // TODO: Cannot validate because of |OpenChannel()| v1 implementation.
  // EXPECT_CALL(this->node_service_observer_, OnNodeFetched(node_id, false));
  EXPECT_CALL(node_observer, OnNodeFetched(node_id, true)).Times(AtMost(1));
  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id))
      .Times(Between(1, 4));
  // TODO: Cannot validate because of |OpenChannel()| v1 implementation.
  // EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(node_id));
  EXPECT_CALL(node_observer,
              OnModelChanged(scada::ModelChangeEvent{
                  node_id, type_definition_id,
                  scada::ModelChangeEvent::ReferenceAdded |
                      scada::ModelChangeEvent::ReferenceDeleted}))
      .Times(AtMost(2));
  EXPECT_CALL(node_observer, OnModelChanged(scada::ModelChangeEvent{
                                 node_id, type_definition_id,
                                 scada::ModelChangeEvent::NodeAdded |
                                     scada::ModelChangeEvent::ReferenceAdded}))
      .Times(AtMost(1));

  this->OpenChannel();

  this->ValidateNodeFetched(node);

  node.Unsubscribe(node_observer);
}

TYPED_TEST(NodeServiceTest, FetchNode) {
  const auto node_id = this->server_address_space_->kTestNode2Id;

  this->OpenChannel();

  EXPECT_CALL(*this->server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(*this->server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());

  auto node = this->node_service_->GetNode(node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  MockFunction<void(const NodeRef& node)> fetch_callback;

  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  this->ValidateNodeFetched(node);

  node.Unsubscribe(node_observer);
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ValidateFetchUnknownNode(
    const scada::NodeId& unknown_node_id) {
  EXPECT_CALL(*this->server_address_space_,
              Read(Each(NodeIs(unknown_node_id)), _))
      .Times(AtMost(1));
  EXPECT_CALL(*this->server_address_space_,
              Browse(Each(NodeIs(unknown_node_id)), _))
      .Times(AtMost(2));
  EXPECT_CALL(this->node_service_observer_,
              OnNodeSemanticChanged(unknown_node_id))
      .Times(AtMost(1));
  EXPECT_CALL(this->node_service_observer_,
              OnNodeFetched(unknown_node_id, true))
      .Times(AtMost(1));

  auto node = this->node_service_->GetNode(unknown_node_id);

  MockFunction<void(const NodeRef& node)> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());
  EXPECT_FALSE(node.status());
}

TYPED_TEST(NodeServiceTest, FetchUnknownNode) {
  this->OpenChannel();

  const scada::NodeId unknown_node_id{1, 100};
  this->ValidateFetchUnknownNode(unknown_node_id);
}

TYPED_TEST(NodeServiceTest, NodeAdded) {
  this->OpenChannel();

  // New node is created with properties having default values.
  const scada::NodeState new_node_state{
      scada::NodeId{111, 1},
      scada::NodeClass::Object,
      this->server_address_space_->kTestTypeId,
      scada::id::RootFolder,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_browse_name("NewNode").set_display_name(
          L"NewNodeDisplayName"),
  };

  this->server_address_space_->CreateNode(new_node_state);

  const scada::ModelChangeEvent node_added_event{
      new_node_state.node_id, new_node_state.type_definition_id,
      scada::ModelChangeEvent::NodeAdded};

  EXPECT_CALL(this->node_service_observer_, OnModelChanged(node_added_event));

  this->view_events_->OnModelChanged(node_added_event);

  Mock::VerifyAndClearExpectations(&this->node_service_observer_);

  EXPECT_CALL(*this->server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(*this->server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  // Fetches the node, its aggregates, and children.
  EXPECT_CALL(*this->server_address_space_,
              Read(Each(NodeIsOrIsNestedOf(new_node_state.node_id)), _))
      .Times(Between(1, 2));
  EXPECT_CALL(*this->server_address_space_,
              Browse(Each(NodeIsOrIsNestedOf(new_node_state.node_id)), _))
      .Times(Between(1, 3));
  EXPECT_CALL(this->node_service_observer_,
              OnModelChanged(scada::ModelChangeEvent{
                  scada::id::RootFolder, scada::id::FolderType,
                  scada::ModelChangeEvent::ReferenceAdded}))
      .Times(AtMost(1));
  EXPECT_CALL(this->node_service_observer_,
              OnModelChanged(scada::ModelChangeEvent{
                  new_node_state.node_id, new_node_state.type_definition_id,
                  scada::ModelChangeEvent::NodeAdded |
                      scada::ModelChangeEvent::ReferenceAdded}))
      .Times(AtMost(1));
  EXPECT_CALL(this->node_service_observer_,
              OnModelChanged(scada::ModelChangeEvent{
                  new_node_state.node_id, new_node_state.type_definition_id,
                  scada::ModelChangeEvent::ReferenceAdded |
                      scada::ModelChangeEvent::ReferenceDeleted}))
      .Times(AtMost(2));
  EXPECT_CALL(this->node_service_observer_,
              OnNodeSemanticChanged(new_node_state.node_id))
      .Times(Between(1, 3));
  EXPECT_CALL(this->node_service_observer_,
              OnNodeFetched(new_node_state.node_id, _))
      .Times(Between(1, 2));

  auto node = this->node_service_->GetNode(new_node_state.node_id);

  MockFunction<void(const NodeRef& node)> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "NewNode");
  EXPECT_EQ(node.display_name(), L"NewNodeDisplayName");
}

TYPED_TEST(NodeServiceTest, NodeDeleted) {
  this->OpenChannel();

  const scada::NodeId deleted_node_id =
      this->server_address_space_->kTestNode1Id;

  EXPECT_CALL(*this->server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(*this->server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());

  auto node = this->node_service_->GetNode(deleted_node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  MockFunction<void(const NodeRef& node)> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  const scada::ModelChangeEvent node_deleted_event{
      deleted_node_id, {}, scada::ModelChangeEvent::NodeDeleted};

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  Mock::VerifyAndClearExpectations(this->server_address_space_.get());
  Mock::VerifyAndClearExpectations(&this->node_service_observer_);

  // ACT

  EXPECT_CALL(this->node_service_observer_, OnModelChanged(node_deleted_event));
  EXPECT_CALL(node_observer, OnModelChanged(node_deleted_event));
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _))
      .Times(AtMost(1));
  EXPECT_CALL(node_observer, OnNodeFetched(_, _)).Times(AtMost(1));

  this->server_address_space_->DeleteNode(deleted_node_id);

  this->view_events_->OnModelChanged(node_deleted_event);

  Mock::VerifyAndClearExpectations(&this->node_service_observer_);

  node.Unsubscribe(node_observer);

  node = nullptr;

  // Now node is unknown.

  this->ValidateFetchUnknownNode(deleted_node_id);
}

TYPED_TEST(NodeServiceTest, NodeSemanticsChanged) {
  this->OpenChannel();

  EXPECT_CALL(*this->server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(*this->server_address_space_, Browse(_, _)).Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_, _))
      .Times(AnyNumber());

  const scada::NodeId changed_node_id =
      this->server_address_space_->kTestNode1Id;
  const scada::Variant new_property_value{"TestNode1.TestProp1.NewValue"};

  auto node = this->node_service_->GetNode(changed_node_id);

  MockFunction<void(const NodeRef& node)> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  Mock::VerifyAndClearExpectations(this->server_address_space_.get());
  Mock::VerifyAndClearExpectations(&this->node_service_observer_);

  // ACT

  const scada::LocalizedText new_display_name{
      base::WideToUTF16(L"NewTestNode1")};

  this->server_address_space_->ModifyNode(
      changed_node_id,
      scada::NodeAttributes{}.set_display_name(new_display_name),
      {{this->server_address_space_->kTestProp1Id, new_property_value}});

  EXPECT_CALL(this->node_service_observer_,
              OnNodeSemanticChanged(changed_node_id));
  EXPECT_CALL(*this->server_address_space_,
              Read(Each(NodeIsOrIsNestedOf(changed_node_id)), _))
      .Times(2);
  EXPECT_CALL(*this->server_address_space_,
              Browse(Each(NodeIsOrIsNestedOf(changed_node_id)), _))
      .Times(2);

  this->view_events_->OnNodeSemanticsChanged(
      scada::SemanticChangeEvent{changed_node_id});

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(new_display_name, node.display_name());
  EXPECT_EQ(new_property_value,
            node[this->server_address_space_->kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode1.TestProp2.Value"},
            node[this->server_address_space_->kTestProp2Id].value());
}
