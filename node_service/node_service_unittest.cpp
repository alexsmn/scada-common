#include "node_service/static/static_node_service.h"
#include "node_service/v1/node_service_impl.h"
#include "node_service/v2/node_service_impl.h"
#include "node_service/v3/node_service_impl.h"

#include "address_space/test/test_address_space.h"
#include "address_space/test/test_matchers.h"
#include "base/test/test_executor.h"
#include "node_service/mock_node_observer.h"
#include "node_service/v1/address_space_fetcher_impl.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"

#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

struct BaseNodeServiceTestEnvironment {
  const std::shared_ptr<TestExecutor> executor;
  const std::shared_ptr<TestAddressSpace> server_address_space;
  const ViewEventsProvider view_events_provider;
};

namespace v1 {

struct NodeServiceTestContext {
  ~NodeServiceTestContext() { client_address_space.Clear(); }

  BaseNodeServiceTestEnvironment& base_env;

  AddressSpaceImpl client_address_space{};

  GenericNodeFactory node_factory{client_address_space};

  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  NiceMock<scada::MockMethodService> method_service;

  NodeServiceImpl node_service{NodeServiceImplContext{
      .address_space_fetcher_factory_ = MakeAddressSpaceFetcherFactory(),
      .address_space_ = *base_env.server_address_space,
      .attribute_service_ = *base_env.server_address_space,
      .monitored_item_service_ = monitored_item_service,
      .method_service_ = method_service}};

 private:
  AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory();
};

AddressSpaceFetcherFactory
NodeServiceTestContext::MakeAddressSpaceFetcherFactory() {
  return [this](AddressSpaceFetcherFactoryContext&& context) {
    return AddressSpaceFetcherImpl::Create(AddressSpaceFetcherImplContext{
        .executor_ = base_env.executor,
        .view_service_ = *base_env.server_address_space,
        .attribute_service_ = *base_env.server_address_space,
        .address_space_ = client_address_space,
        .node_factory_ = node_factory,
        .view_events_provider_ = base_env.view_events_provider,
        .node_fetch_status_changed_handler_ =
            std::move(context.node_fetch_status_changed_handler_),
        .model_changed_handler_ = std::move(context.model_changed_handler_),
        .semantic_changed_handler_ =
            std::move(context.semantic_changed_handler_)});
  };
}

}  // namespace v1

namespace v2 {

struct NodeServiceTestContext {
  BaseNodeServiceTestEnvironment& base_env;

  NiceMock<scada::MockMonitoredItemService> monitored_item_service;

  NodeServiceImpl node_service{NodeServiceImplContext{
      .executor_ = base_env.executor,
      .view_service_ = *base_env.server_address_space,
      .attribute_service_ = *base_env.server_address_space,
      .monitored_item_service_ = monitored_item_service,
      .view_events_provider_ = base_env.view_events_provider}};
};

}  // namespace v2

struct StaticNodeServiceTestContext {
  BaseNodeServiceTestEnvironment& base_env;

  StaticNodeService node_service{};
};

template <class NodeServiceImpl>
class NodeServiceTest : public Test {
 public:
  NodeServiceTest();
  ~NodeServiceTest();

 protected:
  std::shared_ptr<NodeServiceImpl> CreateNodeServiceImpl();
  ViewEventsProvider MakeViewEventsProvider();

  void OpenChannel();

  void ExpectAnyUpdates();
  void ExpectNoUpdates();

  void ValidateFetchUnknownNode(const scada::NodeId& unknown_node_id);
  void ValidateNodeFetched(const NodeRef& node);

  StrictMock<MockNodeObserver> node_service_observer_;

  scada::ViewEvents* view_events_ = nullptr;

  BaseNodeServiceTestEnvironment base_env_{
      .executor = std::make_shared<TestExecutor>(),
      .server_address_space = std::make_shared<TestAddressSpace>(),
      .view_events_provider = MakeViewEventsProvider()};

  const std::shared_ptr<NodeServiceImpl> node_service_ =
      CreateNodeServiceImpl();
};

using NodeServiceImpls =
    Types<v1::NodeServiceImpl, v2::NodeServiceImpl /*, v3::NodeServiceImpl*/>;
TYPED_TEST_SUITE(NodeServiceTest, NodeServiceImpls);

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
  auto context = std::make_shared<v1::NodeServiceTestContext>(base_env_);
  return std::shared_ptr<v1::NodeServiceImpl>(context, &context->node_service);
}

template <>
std::shared_ptr<v2::NodeServiceImpl>
NodeServiceTest<v2::NodeServiceImpl>::CreateNodeServiceImpl() {
  auto context = std::make_shared<v2::NodeServiceTestContext>(base_env_);
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
  node_service_->OnChannelOpened();
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ValidateNodeFetched(
    const NodeRef& node) {
  auto& server_address_space = *this->base_env_.server_address_space;

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());

  // Type definition.
  auto type_definition = node.type_definition();
  EXPECT_TRUE(type_definition);
  EXPECT_TRUE(type_definition.fetched());
  EXPECT_EQ(type_definition.node_id(), server_address_space.kTestTypeId);

  // Supertype.
  auto supertype = type_definition.supertype();
  EXPECT_TRUE(supertype);
  EXPECT_TRUE(supertype.fetched());
  EXPECT_EQ(supertype.node_id(), scada::id::BaseObjectType);

  // Attributes.
  EXPECT_EQ(node.node_class(), scada::NodeClass::Object);
  EXPECT_EQ(node.browse_name(), "TestNode2");
  EXPECT_EQ(node.display_name(), u"TestNode2DisplayName");

  // Properties.
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp1.Value"},
            node[server_address_space.kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode2.TestProp2.Value"},
            node[server_address_space.kTestProp2Id].value());

  // References.
  EXPECT_EQ(node.target(server_address_space.kTestReferenceTypeId).node_id(),
            server_address_space.kTestNode3Id);
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ValidateFetchUnknownNode(
    const scada::NodeId& unknown_node_id) {
  auto& server_address_space = *this->base_env_.server_address_space;

  EXPECT_CALL(server_address_space,
              Read(_, Pointee(Each(NodeIs(unknown_node_id))), _))
      .Times(AtMost(1));

  EXPECT_CALL(server_address_space,
              Browse(/*context=*/_, /*inputs=*/Each(NodeIs(unknown_node_id)),
                     /*callback=*/_))
      .Times(AtMost(2));

  EXPECT_CALL(this->node_service_observer_,
              OnNodeSemanticChanged(unknown_node_id))
      .Times(AtMost(1));

  EXPECT_CALL(
      this->node_service_observer_,
      OnNodeFetched(FieldsAre(unknown_node_id, NodeFetchStatus::None())))
      .Times(AtMost(1));

  auto node = this->node_service_->GetNode(unknown_node_id);

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());
  EXPECT_FALSE(node.status());
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ExpectAnyUpdates() {
  auto& server_address_space = *this->base_env_.server_address_space;

  EXPECT_CALL(server_address_space, Read(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space, Browse(_, _, _)).Times(AnyNumber());
  EXPECT_CALL(node_service_observer_, OnModelChanged(_)).Times(AnyNumber());
  EXPECT_CALL(node_service_observer_, OnNodeSemanticChanged(_))
      .Times(AnyNumber());
  EXPECT_CALL(node_service_observer_, OnNodeFetched(_)).Times(AnyNumber());
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ExpectNoUpdates() {
  auto& server_address_space = *this->base_env_.server_address_space;

  Mock::VerifyAndClearExpectations(&server_address_space);
  Mock::VerifyAndClearExpectations(&node_service_observer_);
}

TYPED_TEST(NodeServiceTest, FetchNode_NodeOnly_WhenChannelClosed) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;
  const auto type_definition_id = server_address_space.kTestTypeId;

  auto node = this->node_service_->GetNode(node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  this->ExpectAnyUpdates();

  EXPECT_CALL(fetch_callback, Call(_));

  EXPECT_CALL(node_observer,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())));

  EXPECT_CALL(this->node_service_observer_,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())));

  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id));
  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(node_id));

  // TOOD: This is not needed. Only v2 triggers this.
  if constexpr (std::is_same_v<TypeParam, v2::NodeServiceImpl>) {
    EXPECT_CALL(node_observer,
                OnModelChanged(scada::ModelChangeEvent{
                    node_id, type_definition_id,
                    scada::ModelChangeEvent::ReferenceAdded |
                        scada::ModelChangeEvent::ReferenceDeleted}));

    EXPECT_CALL(node_observer,
                OnModelChanged(scada::ModelChangeEvent{
                    node_id, type_definition_id,
                    scada::ModelChangeEvent::NodeAdded |
                        scada::ModelChangeEvent::ReferenceAdded}))
        .Times(AtMost(1));
  }

  this->OpenChannel();

  this->ValidateNodeFetched(node);

  node.Unsubscribe(node_observer);
}

TYPED_TEST(NodeServiceTest, FetchNode_NodeOnly) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  EXPECT_CALL(node_observer,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())));

  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id));

  EXPECT_CALL(this->node_service_observer_,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())));

  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(node_id));

  // TODO: Triggered only by v2.
  EXPECT_CALL(node_observer, OnModelChanged(_)).Times(AtMost(1));

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  this->ValidateNodeFetched(node);

  node.Unsubscribe(node_observer);
}

TYPED_TEST(NodeServiceTest, FetchNode_NodeAndChildren) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  ASSERT_FALSE(node.fetched());
  ASSERT_FALSE(node.children_fetched());

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  EXPECT_CALL(node_observer,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())))
      .Times(Between(1, 2));

  // TODO: OnNodeSemanticChanged shouldn't be triggered.
  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id)).Times(AtMost(2));

  // TODO: Triggered only by v2.
  EXPECT_CALL(node_observer, OnModelChanged(_)).Times(AtMost(1));

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeAndChildren(),
             fetch_callback.AsStdFunction());

  this->ValidateNodeFetched(node);

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());

  node.Unsubscribe(node_observer);
}

TYPED_TEST(NodeServiceTest, Fetch_UnknownNode) {
  this->OpenChannel();

  const scada::NodeId unknown_node_id{1, 100};
  this->ValidateFetchUnknownNode(unknown_node_id);
}

TYPED_TEST(NodeServiceTest, NodeAdded) {
  auto& server_address_space = *this->base_env_.server_address_space;

  // New node is created with properties having default values.
  const scada::NodeState new_node_state{
      scada::NodeId{111, 1},
      scada::NodeClass::Object,
      server_address_space.kTestTypeId,
      scada::id::RootFolder,
      scada::id::Organizes,
      scada::NodeAttributes{}.set_browse_name("NewNode").set_display_name(
          u"NewNodeDisplayName"),
  };

  // Process add node notification.

  this->OpenChannel();

  server_address_space.CreateNode(new_node_state);

  const scada::ModelChangeEvent node_added_event{
      new_node_state.node_id, new_node_state.type_definition_id,
      scada::ModelChangeEvent::NodeAdded};

  EXPECT_CALL(this->node_service_observer_, OnModelChanged(node_added_event));

  this->view_events_->OnModelChanged(node_added_event);

  Mock::VerifyAndClearExpectations(&this->node_service_observer_);

  // Pull the added node.

  this->ExpectAnyUpdates();

  // Fetches the node, its aggregates, and children.
  EXPECT_CALL(
      server_address_space,
      Read(_, Pointee(Each(NodeIsOrIsNestedOf(new_node_state.node_id))), _));

  EXPECT_CALL(
      server_address_space,
      Browse(/*context=*/_,
             /*inputs=*/Each(NodeIsOrIsNestedOf(new_node_state.node_id)),
             /*callback=*/_));

  EXPECT_CALL(this->node_service_observer_,
              OnModelChanged(scada::ModelChangeEvent{
                  scada::id::RootFolder, scada::id::FolderType,
                  scada::ModelChangeEvent::ReferenceAdded}))
      .Times(0);

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
      .Times(AtMost(1));

  EXPECT_CALL(this->node_service_observer_,
              OnNodeSemanticChanged(new_node_state.node_id));

  EXPECT_CALL(this->node_service_observer_,
              OnNodeFetched(
                  FieldsAre(new_node_state.node_id, NodeFetchStatus::None())));

  auto node = this->node_service_->GetNode(new_node_state.node_id);

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "NewNode");
  EXPECT_EQ(node.display_name(), u"NewNodeDisplayName");
}

TYPED_TEST(NodeServiceTest, NodeDeleted) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const scada::NodeId& deleted_node_id = server_address_space.kTestNode1Id;

  // INIT

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(deleted_node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  EXPECT_CALL(node_observer, OnNodeFetched(FieldsAre(deleted_node_id,
                                                     NodeFetchStatus::None())));

  EXPECT_CALL(node_observer, OnNodeSemanticChanged(deleted_node_id));

  // TODO: Triggered only by v2.
  EXPECT_CALL(node_observer, OnModelChanged(_)).Times(AtMost(1));

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  this->ExpectNoUpdates();

  // ACT

  server_address_space.DeleteNode(deleted_node_id);

  const scada::ModelChangeEvent node_deleted_event{
      deleted_node_id, {}, scada::ModelChangeEvent::NodeDeleted};

  EXPECT_CALL(this->node_service_observer_, OnModelChanged(node_deleted_event));
  EXPECT_CALL(node_observer, OnModelChanged(node_deleted_event));

  // TODO: Remove. Only v2 triggers it.
  EXPECT_CALL(this->node_service_observer_, OnNodeFetched(_)).Times(AtMost(1));
  EXPECT_CALL(node_observer, OnNodeFetched(_)).Times(AtMost(1));

  this->view_events_->OnModelChanged(node_deleted_event);

  node.Unsubscribe(node_observer);

  node = nullptr;

  this->ExpectNoUpdates();

  // Now node is unknown.

  this->ValidateFetchUnknownNode(deleted_node_id);
}

TYPED_TEST(NodeServiceTest, NodeSemanticsChanged) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const scada::NodeId node_id = server_address_space.kTestNode1Id;
  const scada::LocalizedText new_display_name{u"NewTestNode1"};
  const scada::Variant new_property_value{"TestNode1.TestProp1.NewValue"};

  // INIT

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  EXPECT_CALL(node_observer,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())));

  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id));

  // TODO: Triggered only by v2.
  EXPECT_CALL(node_observer, OnModelChanged(_)).Times(AtMost(1));

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  this->ExpectNoUpdates();

  // ACT

  server_address_space.ModifyNode(
      node_id, scada::NodeAttributes{}.set_display_name(new_display_name),
      {{server_address_space.kTestProp1Id, new_property_value}});

  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(node_id));
  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id));

  EXPECT_CALL(server_address_space,
              Read(_, Pointee(Each(NodeIsOrIsNestedOf(node_id))), _))
      .Times(2);

  EXPECT_CALL(
      server_address_space,
      Browse(/*context=*/_,
             /*inputs=*/Each(NodeIsOrIsNestedOf(node_id)), /*callback=*/_))
      .Times(1);

  this->view_events_->OnNodeSemanticsChanged(
      scada::SemanticChangeEvent{node_id});

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(new_display_name, node.display_name());
  EXPECT_EQ(new_property_value,
            node[server_address_space.kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode1.TestProp2.Value"},
            node[server_address_space.kTestProp2Id].value());

  node.Unsubscribe(node_observer);
}

TYPED_TEST(NodeServiceTest, ReplaceNonHierarchicalReference) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto& node_id = server_address_space.kTestNode2Id;
  const auto& type_definition_id = server_address_space.kTestTypeId;
  const auto& reference_type_id = server_address_space.kTestReferenceTypeId;
  const auto& old_target_node_id = server_address_space.kTestNode3Id;
  const auto& new_target_node_id = server_address_space.kTestNode4Id;

  // INIT

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  StrictMock<MockNodeObserver> node_observer;
  node.Subscribe(node_observer);

  EXPECT_CALL(node_observer,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())));

  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id));

  // TODO: Triggered only by v2.
  EXPECT_CALL(node_observer, OnModelChanged(_)).Times(AtMost(1));

  StrictMock<MockFunction<void(const NodeRef& node)>> fetch_callback;
  EXPECT_CALL(fetch_callback, Call(_));

  node.Fetch(NodeFetchStatus::NodeOnly(), fetch_callback.AsStdFunction());

  ASSERT_EQ(node.target(reference_type_id).node_id(), old_target_node_id);

  // ACT

  scada::DeleteReference(server_address_space, reference_type_id, node_id,
                         old_target_node_id);
  scada::AddReference(server_address_space, reference_type_id, node_id,
                      new_target_node_id);

  // TODO: Disallow this.
  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());

  EXPECT_CALL(this->node_service_observer_, OnModelChanged(_))
      .Times(AnyNumber());

  EXPECT_CALL(server_address_space, Read(_, Pointee(Each(NodeIs(node_id))), _));

  EXPECT_CALL(
      server_address_space,
      Browse(/*context=*/_, /*inputs=*/Each(NodeIs(node_id)), /*callback=*/_))
      .Times(AtMost(2));

  EXPECT_CALL(this->node_service_observer_, OnModelChanged(NodeIs(node_id)))
      .Times(AtMost(3));

  EXPECT_CALL(node_observer, OnModelChanged(NodeIs(node_id))).Times(AtMost(3));

  EXPECT_CALL(this->node_service_observer_, OnNodeSemanticChanged(node_id))
      .Times(AtMost(1));

  EXPECT_CALL(node_observer, OnNodeSemanticChanged(node_id)).Times(AtMost(1));

  // TODO: Shouldn't happen. v1 triggers this.
  EXPECT_CALL(this->node_service_observer_,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())))
      .Times(AtMost(1));

  EXPECT_CALL(node_observer,
              OnNodeFetched(FieldsAre(node_id, NodeFetchStatus::None())))
      .Times(AtMost(1));

  EXPECT_CALL(
      this->node_service_observer_,
      OnNodeFetched(FieldsAre(new_target_node_id, NodeFetchStatus::None())));

  this->view_events_->OnModelChanged(
      scada::ModelChangeEvent{node_id, type_definition_id,
                              scada::ModelChangeEvent::ReferenceAdded |
                                  scada::ModelChangeEvent::ReferenceDeleted});

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.target(reference_type_id).node_id(), new_target_node_id);

  node.Unsubscribe(node_observer);
}

TYPED_TEST(NodeServiceTest,
           NonHierarchicalForwardReferencesAreFetchedAlongWithNode) {
  this->ExpectAnyUpdates();
  this->OpenChannel();

  auto& server_address_space = *this->base_env_.server_address_space;
  auto node_id = server_address_space.kTestNode2Id;
  auto node = this->node_service_->GetNode(node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());

  auto target = node.target(server_address_space.kTestReferenceTypeId);
  EXPECT_EQ(target.node_id(), server_address_space.kTestNode3Id);
  EXPECT_TRUE(target.fetched());
  EXPECT_TRUE(target.status());
}

TYPED_TEST(NodeServiceTest, TsFormat) {
  this->ExpectAnyUpdates();
  this->OpenChannel();

  auto& server_address_space = *this->base_env_.server_address_space;
  auto node_id = server_address_space.kTestNode2Id;
  auto node = this->node_service_->GetNode(node_id);
  node.Fetch(NodeFetchStatus::NodeOnly());
}
