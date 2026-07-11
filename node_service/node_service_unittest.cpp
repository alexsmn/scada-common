#include "base/check.h"
#include "node_service/static/static_node_service.h"
#include "node_service/v1/node_service_impl.h"
#include "node_service/v2/node_service_impl.h"
#include "node_service/v3/node_service_impl.h"

#include "address_space/node_utils.h"
#include "address_space/test/test_address_space.h"
#include "address_space/test/test_matchers.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "node_service/test/recording_node_observer.h"
#include "node_service/v1/address_space_fetcher_impl.h"
#include "node_service/v1/address_space_method_service.h"
#include "node_service/v3/node_fetcher.h"
#include "scada/attribute_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/view_service.h"

#include <gmock/gmock.h>

#include "base/debug_util.h"

using namespace testing;

TEST(AddressSpaceMethodServiceTest, CoroutineCallReturnsWrongMethodId) {
  TestExecutor executor;
  TestAddressSpace address_space;
  AddressSpaceMethodService service{address_space};

  const auto status = WaitAwaitable(
      executor, service.Call(scada::NodeId{1, 2}, scada::NodeId{1, 3}, {}, {}));

  EXPECT_EQ(status.code(), scada::StatusCode::Bad_WrongMethodId);
}

struct BaseNodeServiceTestEnvironment {
  TestExecutor executor;
  const std::shared_ptr<TestAddressSpace> server_address_space;
  const ViewEventsProvider view_events_provider;
};

namespace v1 {

struct NodeServiceTestContext {
  ~NodeServiceTestContext() { client_address_space.Clear(); }

  BaseNodeServiceTestEnvironment& base_env;

  AddressSpaceImpl client_address_space{};

  GenericNodeFactory node_factory{client_address_space};

  NodeServiceImpl node_service{NodeServiceImplContext{
      .address_space_fetcher_factory_ = MakeAddressSpaceFetcherFactory(),
      .address_space_ = *base_env.server_address_space,
      .scada_client_ = {}}};

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

namespace v3 {

class TestNodeFetcher final : public NodeFetcher {
 public:
  explicit TestNodeFetcher(TestAddressSpace& address_space)
      : address_space_{address_space} {}

  Awaitable<scada::StatusOr<scada::NodeState>> FetchNode(
      const scada::NodeId& node_id) override {
    const auto* node = address_space_.GetNode(node_id);
    if (!node) {
      co_return scada::StatusCode::Bad_WrongNodeId;
    }
    co_return scada::MakeNodeState(*node);
  }

  Awaitable<scada::StatusOr<scada::ReferenceDescriptions>> FetchChildren(
      const scada::NodeId& node_id) override {
    const auto* node = address_space_.GetNode(node_id);
    if (!node) {
      co_return scada::StatusCode::Bad_WrongNodeId;
    }

    scada::ReferenceDescriptions references;
    for (const auto& reference : node->forward_references()) {
      if (scada::IsSubtypeOf(*reference.type,
                             scada::id::HierarchicalReferences)) {
        references.push_back({.reference_type_id = reference.type->id(),
                              .forward = true,
                              .node_id = reference.node->id()});
      }
    }
    co_return references;
  }

 private:
  TestAddressSpace& address_space_;
};

struct NodeServiceTestContext {
  BaseNodeServiceTestEnvironment& base_env;

  // Large by default so ordinary read-after-fetch tests keep the fetched
  // entry resident (NodeRefs no longer pin under the cursor model). Residency
  // and eviction tests build their own context with a small/zero capacity and
  // pin the node under test with a subscription.
  size_t keep_alive_capacity = 1024;

  NiceMock<scada::MockMonitoredItemService> monitored_item_service;
  std::shared_ptr<TestNodeFetcher> node_fetcher{
      std::make_shared<TestNodeFetcher>(*base_env.server_address_space)};

  NodeServiceImpl node_service{NodeServiceImplContext{
      .executor_ = base_env.executor,
      .monitored_item_service_ = monitored_item_service,
      .node_fetcher_ = node_fetcher,
      .view_events_provider_ = base_env.view_events_provider,
      .keep_alive_capacity_ = keep_alive_capacity}};
};

}  // namespace v3

struct StaticNodeServiceTestContext {
  BaseNodeServiceTestEnvironment& base_env;

  StaticNodeService node_service{};
};

TEST(StaticNodeServiceTest, ScadaNodeUsesDataServicesAttributeService) {
  TestExecutor executor;
  StrictMock<scada::MockAttributeService> attribute_service;
  DataServices data_services;
  data_services.attribute_service_ = std::shared_ptr<scada::AttributeService>{
      std::shared_ptr<void>{}, &attribute_service};
  StaticNodeService node_service{std::move(data_services)};
  const scada::NodeId node_id{1, 2};
  const scada::Variant expected_value{scada::Int32{42}};

  node_service.Add({.node_id = node_id,
                    .node_class = scada::NodeClass::Variable,
                    .type_definition_id = scada::id::BaseVariableType});

  EXPECT_CALL(attribute_service, Read(_, _))
      .WillOnce(
          [&](scada::ServiceContext, std::vector<scada::ReadValueId> inputs)
              -> Awaitable<scada::StatusOr<std::vector<scada::DataValue>>> {
            EXPECT_EQ(inputs.size(), 1u);
            if (inputs.empty()) {
              co_return scada::StatusCode::Bad;
            }
            EXPECT_EQ(inputs[0].node_id, node_id);
            EXPECT_EQ(inputs[0].attribute_id, scada::AttributeId::Value);
            co_return std::vector{scada::DataValue{expected_value, {}, {}, {}}};
          });

  auto result = WaitAwaitable(
      executor, node_service.GetNode(node_id).scada_node().read_value());

  EXPECT_EQ(result.value().value, expected_value);
}

template <class NodeServiceImpl>
class NodeServiceTest : public Test {
 public:
  NodeServiceTest();
  ~NodeServiceTest();

 protected:
  std::shared_ptr<NodeServiceImpl> CreateNodeServiceImpl();
  ViewEventsProvider MakeViewEventsProvider();

  void OpenChannel();
  void DrainExecutor();

  void ExpectAnyUpdates();
  void ExpectNoUpdates();

  void ValidateFetchUnknownNode(const scada::NodeId& unknown_node_id);
  void ValidateNodeFetched(const NodeRef& node);

  RecordingNodeObserver node_service_observer_;

  scada::ViewEvents* view_events_ = nullptr;

  BaseNodeServiceTestEnvironment base_env_{
      .executor = TestExecutor{},
      .server_address_space = std::make_shared<TestAddressSpace>(),
      .view_events_provider = MakeViewEventsProvider()};

  const std::shared_ptr<NodeServiceImpl> node_service_ =
      CreateNodeServiceImpl();
};

using NodeServiceImpls = Types<v1::NodeServiceImpl, v2::NodeServiceImpl>;
TYPED_TEST_SUITE(NodeServiceTest, NodeServiceImpls);

template <class NodeServiceImpl>
class CommonNodeServiceTest : public NodeServiceTest<NodeServiceImpl> {};

using CommonNodeServiceImpls =
    Types<v1::NodeServiceImpl, v2::NodeServiceImpl, v3::NodeServiceImpl>;
TYPED_TEST_SUITE(CommonNodeServiceTest, CommonNodeServiceImpls);

template <class NodeServiceImpl>
NodeServiceTest<NodeServiceImpl>::NodeServiceTest() {
  node_service_observer_.Connect(*node_service_);
}

template <class NodeServiceImpl>
NodeServiceTest<NodeServiceImpl>::~NodeServiceTest() {
  node_service_observer_.Disconnect();
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

template <>
std::shared_ptr<v3::NodeServiceImpl>
NodeServiceTest<v3::NodeServiceImpl>::CreateNodeServiceImpl() {
  auto context = std::make_shared<v3::NodeServiceTestContext>(base_env_);
  return std::shared_ptr<v3::NodeServiceImpl>(context, &context->node_service);
}

template <class NodeServiceImpl>
ViewEventsProvider NodeServiceTest<NodeServiceImpl>::MakeViewEventsProvider() {
  return [this](scada::ViewEvents& events)
             -> std::unique_ptr<IViewEventsSubscription> {
    base::Check(!view_events_);
    view_events_ = &events;
    return std::make_unique<IViewEventsSubscription>();
  };
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::OpenChannel() {
  node_service_->OnChannelOpened();
  DrainExecutor();
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::DrainExecutor() {
  for (size_t i = 0; i < 100 && base_env_.executor.GetTaskCount() != 0; ++i)
    base_env_.executor.Poll();
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ValidateNodeFetched(
    const NodeRef& node) {
  DrainExecutor();

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

  EXPECT_CALL(server_address_space, Read(_, Each(NodeIs(unknown_node_id))))
      .Times(AtMost(1));

  EXPECT_CALL(server_address_space,
              Browse(/*context=*/_, /*inputs=*/Each(NodeIs(unknown_node_id))))
      .Times(AtMost(2));

  node_service_observer_.ClearEvents();

  auto node = this->node_service_->GetNode(unknown_node_id);

  node.StartFetch(NodeFetchStatus::NodeOnly());
  DrainExecutor();

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());
  EXPECT_FALSE(node.status());

  // Only the fetch-error notifications for the unknown node are allowed.
  EXPECT_THAT(node_service_observer_.model_change_events, IsEmpty());
  EXPECT_THAT(node_service_observer_.state_changed_events, IsEmpty());
  EXPECT_LE(node_service_observer_.semantic_changed_node_ids.size(), 1u);
  EXPECT_THAT(node_service_observer_.semantic_changed_node_ids,
              Each(unknown_node_id));
  EXPECT_LE(node_service_observer_.node_fetched_events.size(), 1u);
  EXPECT_THAT(node_service_observer_.node_fetched_events,
              Each(FieldsAre(unknown_node_id, NodeFetchStatus::None())));
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ExpectAnyUpdates() {
  auto& server_address_space = *this->base_env_.server_address_space;

  EXPECT_CALL(server_address_space, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(server_address_space, Browse(_, _)).Times(AnyNumber());
}

template <class NodeServiceImpl>
void NodeServiceTest<NodeServiceImpl>::ExpectNoUpdates() {
  DrainExecutor();

  auto& server_address_space = *this->base_env_.server_address_space;

  Mock::VerifyAndClearExpectations(&server_address_space);
  node_service_observer_.ClearEvents();
}

TYPED_TEST(NodeServiceTest, FetchNode_NodeOnly_WhenChannelClosed) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;
  const auto type_definition_id = server_address_space.kTestTypeId;

  auto node = this->node_service_->GetNode(node_id);

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  node.StartFetch(NodeFetchStatus::NodeOnly());

  this->ExpectAnyUpdates();

  this->OpenChannel();

  this->ValidateNodeFetched(node);

  EXPECT_THAT(node_observer.node_fetched_events,
              ElementsAre(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_THAT(node_observer.semantic_changed_node_ids, ElementsAre(node_id));
  EXPECT_THAT(node_observer.state_changed_events, IsEmpty());

  // TOOD: This is not needed. Only v2 triggers this.
  if constexpr (std::is_same_v<TypeParam, v2::NodeServiceImpl>) {
    const scada::ModelChangeEvent references_changed_event{
        node_id, type_definition_id,
        scada::ModelChangeEvent::ReferenceAdded |
            scada::ModelChangeEvent::ReferenceDeleted};
    const scada::ModelChangeEvent node_added_event{
        node_id, type_definition_id,
        scada::ModelChangeEvent::NodeAdded |
            scada::ModelChangeEvent::ReferenceAdded};

    EXPECT_THAT(node_observer.model_change_events,
                Contains(references_changed_event).Times(1));
    EXPECT_THAT(node_observer.model_change_events,
                Each(AnyOf(references_changed_event, node_added_event)));
  } else {
    EXPECT_THAT(node_observer.model_change_events, IsEmpty());
  }

  EXPECT_THAT(this->node_service_observer_.node_fetched_events,
              Contains(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_THAT(this->node_service_observer_.semantic_changed_node_ids,
              Contains(node_id));
}

TYPED_TEST(CommonNodeServiceTest, FetchNode_NodeOnly_FetchesCoreAttributes) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();
  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  node.StartFetch(NodeFetchStatus::NodeOnly());
  this->DrainExecutor();

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.node_class(), scada::NodeClass::Object);
  EXPECT_EQ(node.browse_name(), "TestNode2");
  EXPECT_EQ(node.display_name(), u"TestNode2DisplayName");
}

TYPED_TEST(NodeServiceTest, FetchNode_NodeOnly) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  node.StartFetch(NodeFetchStatus::NodeOnly());

  this->ValidateNodeFetched(node);

  EXPECT_THAT(node_observer.node_fetched_events,
              ElementsAre(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_THAT(node_observer.semantic_changed_node_ids, ElementsAre(node_id));
  // TODO: Triggered only by v2.
  EXPECT_LE(node_observer.model_change_events.size(), 1u);
  EXPECT_THAT(node_observer.state_changed_events, IsEmpty());

  EXPECT_THAT(this->node_service_observer_.node_fetched_events,
              Contains(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_THAT(this->node_service_observer_.semantic_changed_node_ids,
              Contains(node_id));
}

TYPED_TEST(NodeServiceTest, FetchNode_NodeAndChildren) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(node_id);

  ASSERT_FALSE(node.fetched());
  ASSERT_FALSE(node.children_fetched());

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  node.StartFetch(NodeFetchStatus::NodeAndChildren());

  this->ValidateNodeFetched(node);

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());

  EXPECT_THAT(node_observer.node_fetched_events,
              Each(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_GE(node_observer.node_fetched_events.size(), 1u);
  EXPECT_LE(node_observer.node_fetched_events.size(), 2u);

  // TODO: OnNodeSemanticChanged shouldn't be triggered.
  EXPECT_THAT(node_observer.semantic_changed_node_ids, Each(node_id));
  EXPECT_LE(node_observer.semantic_changed_node_ids.size(), 2u);

  // TODO: Triggered only by v2.
  EXPECT_LE(node_observer.model_change_events.size(), 1u);
  EXPECT_THAT(node_observer.state_changed_events, IsEmpty());
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

  this->view_events_->OnModelChanged(node_added_event);

  EXPECT_THAT(this->node_service_observer_.model_change_events,
              ElementsAre(node_added_event));
  this->node_service_observer_.ClearEvents();

  // Pull the added node.

  this->ExpectAnyUpdates();

  // Fetches the node, its aggregates, and children.
  EXPECT_CALL(server_address_space,
              Read(_, Each(NodeIsOrIsNestedOf(new_node_state.node_id))));

  EXPECT_CALL(
      server_address_space,
      Browse(/*context=*/_,
             /*inputs=*/Each(NodeIsOrIsNestedOf(new_node_state.node_id))));

  auto node = this->node_service_->GetNode(new_node_state.node_id);

  node.StartFetch(NodeFetchStatus::NodeOnly());
  this->DrainExecutor();

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.browse_name(), "NewNode");
  EXPECT_EQ(node.display_name(), u"NewNodeDisplayName");

  EXPECT_THAT(this->node_service_observer_.model_change_events,
              Not(Contains(scada::ModelChangeEvent{
                  scada::id::RootFolder, scada::id::FolderType,
                  scada::ModelChangeEvent::ReferenceAdded})));

  EXPECT_THAT(this->node_service_observer_.semantic_changed_node_ids,
              Contains(new_node_state.node_id));

  EXPECT_THAT(
      this->node_service_observer_.node_fetched_events,
      Contains(FieldsAre(new_node_state.node_id, NodeFetchStatus::None())));
}

TYPED_TEST(NodeServiceTest, NodeDeleted) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const scada::NodeId& deleted_node_id = server_address_space.kTestNode1Id;

  // INIT

  this->OpenChannel();

  this->ExpectAnyUpdates();

  auto node = this->node_service_->GetNode(deleted_node_id);

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  node.StartFetch(NodeFetchStatus::NodeOnly());

  this->ExpectNoUpdates();

  EXPECT_THAT(node_observer.node_fetched_events,
              ElementsAre(FieldsAre(deleted_node_id, NodeFetchStatus::None())));
  EXPECT_THAT(node_observer.semantic_changed_node_ids,
              ElementsAre(deleted_node_id));
  // TODO: Triggered only by v2.
  EXPECT_LE(node_observer.model_change_events.size(), 1u);
  node_observer.ClearEvents();

  // ACT

  server_address_space.DeleteNode(deleted_node_id);

  const scada::ModelChangeEvent node_deleted_event{
      deleted_node_id, {}, scada::ModelChangeEvent::NodeDeleted};

  this->view_events_->OnModelChanged(node_deleted_event);

  EXPECT_THAT(this->node_service_observer_.model_change_events,
              ElementsAre(node_deleted_event));
  EXPECT_THAT(node_observer.model_change_events,
              ElementsAre(node_deleted_event));

  // TODO: Remove. Only v2 triggers it.
  EXPECT_LE(this->node_service_observer_.node_fetched_events.size(), 1u);
  EXPECT_LE(node_observer.node_fetched_events.size(), 1u);

  node_observer.Disconnect();

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

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  node.StartFetch(NodeFetchStatus::NodeOnly());

  this->ExpectNoUpdates();

  EXPECT_THAT(node_observer.node_fetched_events,
              ElementsAre(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_THAT(node_observer.semantic_changed_node_ids, ElementsAre(node_id));
  // TODO: Triggered only by v2.
  EXPECT_LE(node_observer.model_change_events.size(), 1u);
  node_observer.ClearEvents();

  // ACT

  server_address_space.ModifyNode(
      node_id, scada::NodeAttributes{}.set_display_name(new_display_name),
      {{server_address_space.kTestProp1Id, new_property_value}});

  EXPECT_CALL(server_address_space, Read(_, Each(NodeIsOrIsNestedOf(node_id))))
      .Times(2);

  EXPECT_CALL(server_address_space,
              Browse(/*context=*/_,
                     /*inputs=*/Each(NodeIsOrIsNestedOf(node_id))))
      .Times(1);

  this->view_events_->OnNodeSemanticsChanged(
      scada::SemanticChangeEvent{node_id});
  this->DrainExecutor();

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(new_display_name, node.display_name());
  EXPECT_EQ(new_property_value,
            node[server_address_space.kTestProp1Id].value());
  EXPECT_EQ(scada::Variant{"TestNode1.TestProp2.Value"},
            node[server_address_space.kTestProp2Id].value());

  EXPECT_THAT(this->node_service_observer_.semantic_changed_node_ids,
              ElementsAre(node_id));
  EXPECT_THAT(node_observer.semantic_changed_node_ids, ElementsAre(node_id));
  EXPECT_THAT(node_observer.node_fetched_events, IsEmpty());
  EXPECT_THAT(node_observer.model_change_events, IsEmpty());
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

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  node.StartFetch(NodeFetchStatus::NodeOnly());
  this->DrainExecutor();

  ASSERT_EQ(node.target(reference_type_id).node_id(), old_target_node_id);

  EXPECT_THAT(node_observer.node_fetched_events,
              ElementsAre(FieldsAre(node_id, NodeFetchStatus::None())));
  EXPECT_THAT(node_observer.semantic_changed_node_ids, ElementsAre(node_id));
  // TODO: Triggered only by v2.
  EXPECT_LE(node_observer.model_change_events.size(), 1u);
  node_observer.ClearEvents();
  this->node_service_observer_.ClearEvents();

  // ACT

  scada::DeleteReference(server_address_space, reference_type_id, node_id,
                         old_target_node_id);
  scada::AddReference(server_address_space, reference_type_id, node_id,
                      new_target_node_id);

  EXPECT_CALL(server_address_space, Read(_, Each(NodeIs(node_id))));

  EXPECT_CALL(server_address_space,
              Browse(/*context=*/_, /*inputs=*/Each(NodeIs(node_id))))
      .Times(AtMost(2));

  this->view_events_->OnModelChanged(
      scada::ModelChangeEvent{node_id, type_definition_id,
                              scada::ModelChangeEvent::ReferenceAdded |
                                  scada::ModelChangeEvent::ReferenceDeleted});
  this->DrainExecutor();

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.target(reference_type_id).node_id(), new_target_node_id);

  // TODO: Disallow the unmatched model-change events.
  EXPECT_LE(node_observer.model_change_events.size(), 3u);
  EXPECT_THAT(node_observer.model_change_events, Each(NodeIs(node_id)));

  EXPECT_LE(node_observer.semantic_changed_node_ids.size(), 1u);
  EXPECT_THAT(node_observer.semantic_changed_node_ids, Each(node_id));

  // The replacement target must be reported as fetched. (Service-wide
  // fetch/semantic notifications for neighboring nodes are impl-specific
  // and deliberately not constrained here.)
  EXPECT_THAT(this->node_service_observer_.node_fetched_events,
              Contains(FieldsAre(new_target_node_id, NodeFetchStatus::None())));
  // TODO: The (node_id, None) refetch notification shouldn't happen; v1
  // triggers it.
  EXPECT_LE(node_observer.node_fetched_events.size(), 1u);
  EXPECT_THAT(node_observer.node_fetched_events,
              Each(FieldsAre(node_id, NodeFetchStatus::None())));
}

TYPED_TEST(NodeServiceTest,
           NonHierarchicalForwardReferencesAreFetchedAlongWithNode) {
  this->ExpectAnyUpdates();
  this->OpenChannel();

  auto& server_address_space = *this->base_env_.server_address_space;
  auto node_id = server_address_space.kTestNode2Id;
  auto node = this->node_service_->GetNode(node_id);
  node.StartFetch(NodeFetchStatus::NodeOnly());
  this->DrainExecutor();

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
  node.StartFetch(NodeFetchStatus::NodeOnly());
  this->DrainExecutor();
}

// v3-specific residency and snapshot tests. GetResidentNodeCount is only
// exposed by v3::NodeServiceImpl, so these are not part of the typed suites.
class V3NodeServiceTest : public NodeServiceTest<v3::NodeServiceImpl> {
 public:
  V3NodeServiceTest() {
    ExpectAnyUpdates();
    observer_.Connect(*node_service_);
  }

 protected:
  RecordingNodeObserver observer_;
};

TEST_F(V3NodeServiceTest, FetchDoesNotMaterializeReferenceNeighborhood) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  // GetNode starts a NodeOnly fetch; TestNode2 carries references to its
  // parent, type definition, properties and TestNode3.
  auto node = this->node_service_->GetNode(node_id);
  this->DrainExecutor();

  EXPECT_TRUE(node.fetched());

  // Fetching a node must not create models for its reference neighborhood.
  EXPECT_EQ(this->node_service_->GetResidentNodeCount(), 1u);
}

TEST_F(V3NodeServiceTest, ServiceObserverSeesEventsForNonResidentNodes) {
  this->OpenChannel();

  const scada::NodeId node_id{1, 100};
  const scada::ModelChangeEvent reference_changed_event{
      node_id, {}, scada::ModelChangeEvent::ReferenceAdded};
  const scada::ModelChangeEvent node_added_event{
      node_id, {}, scada::ModelChangeEvent::NodeAdded};

  this->view_events_->OnModelChanged(reference_changed_event);
  this->view_events_->OnModelChanged(node_added_event);
  this->view_events_->OnNodeSemanticsChanged(
      scada::SemanticChangeEvent{node_id});

  EXPECT_THAT(this->observer_.model_change_events,
              ElementsAre(reference_changed_event, node_added_event));
  EXPECT_THAT(this->observer_.semantic_changed_node_ids, ElementsAre(node_id));

  // Event routing must not create models either.
  EXPECT_EQ(this->node_service_->GetResidentNodeCount(), 0u);
}

TEST_F(V3NodeServiceTest, NodeDeletedTombstonesResidentModel) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode1Id;

  this->OpenChannel();

  auto node = this->node_service_->GetNode(node_id);
  this->DrainExecutor();
  ASSERT_TRUE(node.fetched());
  ASSERT_TRUE(node.status());

  RecordingNodeObserver node_observer;
  node_observer.Connect(node);

  const scada::ModelChangeEvent node_deleted_event{
      node_id, {}, scada::ModelChangeEvent::NodeDeleted};
  this->view_events_->OnModelChanged(node_deleted_event);

  // Per-node observers see the deletion; the held model is tombstoned:
  // still fetched, but with an error status.
  EXPECT_THAT(node_observer.model_change_events,
              ElementsAre(node_deleted_event));
  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
}

TEST_F(V3NodeServiceTest, FetchUnknownNodeReportsError) {
  const scada::NodeId unknown_node_id{1, 100};

  this->OpenChannel();

  auto node = this->node_service_->GetNode(unknown_node_id);
  this->DrainExecutor();

  // The error lands on the requesting (resident) model without creating any
  // other models as a side effect.
  EXPECT_TRUE(node.fetched());
  EXPECT_FALSE(node.status());
  EXPECT_EQ(this->node_service_->GetResidentNodeCount(), 1u);
}

TEST_F(V3NodeServiceTest, NeighborSnapshotRepublishedOnInverseReferencePush) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto neighbor_id = server_address_space.kTestNode3Id;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  auto neighbor = this->node_service_->GetNode(neighbor_id);
  this->DrainExecutor();

  const scada::NodeStatePtr old_state = this->observer_.last_state(neighbor_id);
  ASSERT_NE(old_state, nullptr);

  // TestNode2 references TestNode3; fetching it pushes the inverse reference
  // into the resident neighbor, which republishes its snapshot.
  auto node = this->node_service_->GetNode(node_id);
  this->DrainExecutor();

  const scada::NodeStatePtr new_state = this->observer_.last_state(neighbor_id);
  ASSERT_NE(new_state, nullptr);
  EXPECT_NE(new_state, old_state);

  bool has_inverse_reference = false;
  for (const auto& reference : new_state->references) {
    if (!reference.forward && reference.node_id == node_id)
      has_inverse_reference = true;
  }
  EXPECT_TRUE(has_inverse_reference);
}

TEST_F(V3NodeServiceTest, PublishesImmutableSnapshotOnFetch) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  auto node = this->node_service_->GetNode(node_id);
  this->DrainExecutor();

  ASSERT_TRUE(node.fetched());

  // Exactly one snapshot is published per fetch batch, and it carries the
  // fetched node data.
  ASSERT_EQ(this->observer_.state_changed_events.size(), 1u);
  const auto& event = this->observer_.state_changed_events.front();
  EXPECT_EQ(event.node_id, node_id);
  EXPECT_TRUE(event.fetch_status.node_fetched);
  ASSERT_NE(event.state, nullptr);
  EXPECT_EQ(event.state->node_id, node_id);
  EXPECT_EQ(event.state->attributes.browse_name, "TestNode2");
}

TEST_F(V3NodeServiceTest, SnapshotIsCopyOnWrite) {
  auto& server_address_space = *this->base_env_.server_address_space;
  const auto node_id = server_address_space.kTestNode2Id;

  this->OpenChannel();

  auto node = this->node_service_->GetNode(node_id);
  this->DrainExecutor();

  const scada::NodeStatePtr old_state = this->observer_.last_state(node_id);
  ASSERT_NE(old_state, nullptr);
  const size_t old_reference_count = old_state->references.size();

  // Children fetch publishes a new self-contained snapshot that includes the
  // child references; the previously published snapshot stays frozen.
  node.StartFetch(NodeFetchStatus::NodeAndChildren());
  this->DrainExecutor();

  const scada::NodeStatePtr new_state = this->observer_.last_state(node_id);
  ASSERT_NE(new_state, nullptr);
  EXPECT_NE(new_state, old_state);
  EXPECT_EQ(old_state->references.size(), old_reference_count);
  EXPECT_GT(new_state->references.size(), old_reference_count);
}

// Keep-alive tests need a non-zero capacity, so they build their own context
// instead of using the V3NodeServiceTest fixture.
class V3KeepAliveTest : public Test {
 protected:
  void DrainExecutor() {
    for (size_t i = 0; i < 100 && base_env_.executor.GetTaskCount() != 0; ++i)
      base_env_.executor.Poll();
  }

  scada::ViewEvents* view_events_ = nullptr;

  BaseNodeServiceTestEnvironment base_env_{
      .executor = TestExecutor{},
      .server_address_space = std::make_shared<TestAddressSpace>(),
      .view_events_provider = [this](scada::ViewEvents& events)
          -> std::unique_ptr<IViewEventsSubscription> {
        view_events_ = &events;
        return std::make_unique<IViewEventsSubscription>();
      }};

  v3::NodeServiceTestContext context_{base_env_, /*keep_alive_capacity=*/2};
};

TEST_F(V3KeepAliveTest, KeepAliveWindowRetainsRecentModels) {
  auto& server_address_space = *base_env_.server_address_space;
  auto& node_service = context_.node_service;

  node_service.OnChannelOpened();

  // All NodeRefs are dropped immediately; only the 2-slot keep-alive window
  // holds models after the fetches complete.
  node_service.GetNode(server_address_space.kTestNode1Id);
  node_service.GetNode(server_address_space.kTestNode2Id);
  node_service.GetNode(server_address_space.kTestNode3Id);
  DrainExecutor();

  EXPECT_EQ(node_service.GetResidentNodeCount(), 2u);
}

TEST_F(V3KeepAliveTest, NodeDeletedEvictsKeepAlivePin) {
  auto& server_address_space = *base_env_.server_address_space;
  auto& node_service = context_.node_service;
  const auto node_id = server_address_space.kTestNode1Id;

  node_service.OnChannelOpened();

  node_service.GetNode(node_id);
  DrainExecutor();
  ASSERT_EQ(node_service.GetResidentNodeCount(), 1u);

  view_events_->OnModelChanged(scada::ModelChangeEvent{
      node_id, {}, scada::ModelChangeEvent::NodeDeleted});

  EXPECT_EQ(node_service.GetResidentNodeCount(), 0u);
}

// Residency semantics under the cursor model: NodeRefs no longer pin, so a
// node stays resident only via keep-alive (disabled here), an in-flight fetch,
// a pending pre-channel fetch, or an active subscription. Uses a zero-capacity
// keep-alive so those pins are observed directly.
class V3ResidencyTest : public Test {
 protected:
  void DrainExecutor() {
    for (size_t i = 0; i < 100 && base_env_.executor.GetTaskCount() != 0; ++i)
      base_env_.executor.Poll();
  }

  scada::ViewEvents* view_events_ = nullptr;

  BaseNodeServiceTestEnvironment base_env_{
      .executor = TestExecutor{},
      .server_address_space = std::make_shared<TestAddressSpace>(),
      .view_events_provider = [this](scada::ViewEvents& events)
          -> std::unique_ptr<IViewEventsSubscription> {
        view_events_ = &events;
        return std::make_unique<IViewEventsSubscription>();
      }};

  v3::NodeServiceTestContext context_{base_env_, /*keep_alive_capacity=*/0};
  v3::NodeServiceImpl& node_service_ = context_.node_service;
};

// A cursor alone no longer pins its node: once the initiating fetch completes
// and nothing else holds the model, it is released.
TEST_F(V3ResidencyTest, CursorAloneDoesNotPinNode) {
  auto& server_address_space = *base_env_.server_address_space;

  node_service_.OnChannelOpened();

  auto node = node_service_.GetNode(server_address_space.kTestNode1Id);
  DrainExecutor();

  // The cursor `node` is still in scope, yet the model is gone.
  EXPECT_EQ(node_service_.GetResidentNodeCount(), 0u);
}

// An active subscription keeps the node resident for the connection's lifetime.
TEST_F(V3ResidencyTest, SubscriptionPinsNodeUntilDisconnected) {
  auto& server_address_space = *base_env_.server_address_space;

  node_service_.OnChannelOpened();

  auto node = node_service_.GetNode(server_address_space.kTestNode1Id);
  RecordingNodeObserver observer;
  observer.Connect(node);  // node-scoped subscriptions pin the model
  DrainExecutor();

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 1u);

  observer.Disconnect();

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 0u);
}

// An in-flight fetch pins its target even if the caller drops the cursor.
TEST_F(V3ResidencyTest, InFlightFetchPinsModel) {
  auto& server_address_space = *base_env_.server_address_space;

  node_service_.OnChannelOpened();

  node_service_.GetNode(server_address_space.kTestNode2Id);  // cursor dropped

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 1u);

  DrainExecutor();

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 0u);
}

// A fetch queued before the channel opens pins its model until it can run.
TEST_F(V3ResidencyTest, PendingFetchPinsModel) {
  auto& server_address_space = *base_env_.server_address_space;

  node_service_.GetNode(server_address_space.kTestNode2Id);  // channel closed

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 1u);

  node_service_.OnChannelOpened();
  DrainExecutor();

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 0u);
}

// Reading through a cursor whose entry was evicted transparently re-fetches:
// the read re-creates the entry and kicks a NodeOnly fetch, which pins it.
TEST_F(V3ResidencyTest, ReadReFetchesEvictedNode) {
  auto& server_address_space = *base_env_.server_address_space;

  node_service_.OnChannelOpened();

  auto node = node_service_.GetNode(server_address_space.kTestNode1Id);
  DrainExecutor();
  ASSERT_EQ(node_service_.GetResidentNodeCount(), 0u);  // evicted

  node.display_name();  // a read on the evicted cursor

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 1u);  // re-fetch in flight

  DrainExecutor();

  EXPECT_EQ(node_service_.GetResidentNodeCount(), 0u);
}

class V2NodeServiceRegressionTest : public Test {
 public:
  V2NodeServiceRegressionTest() { node_observer_.Connect(*node_service_); }

 protected:
  ViewEventsProvider MakeViewEventsProvider() {
    return [this](scada::ViewEvents& events)
               -> std::unique_ptr<IViewEventsSubscription> {
      view_events_ = &events;
      return std::make_unique<IViewEventsSubscription>();
    };
  }

  void DrainExecutor() {
    for (size_t i = 0; i < 100 && executor_.GetTaskCount() != 0; ++i)
      executor_.Poll();
  }

  void OpenChannel() {
    node_service_->OnChannelOpened();
    DrainExecutor();
  }

  scada::ViewEvents* view_events_ = nullptr;
  TestExecutor executor_;
  const std::shared_ptr<TestAddressSpace> server_address_space_ =
      std::make_shared<TestAddressSpace>();
  BaseNodeServiceTestEnvironment base_env_{
      .executor = executor_,
      .server_address_space = server_address_space_,
      .view_events_provider = MakeViewEventsProvider()};
  RecordingNodeObserver node_observer_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  std::shared_ptr<v2::NodeServiceImpl> node_service_ =
      std::make_shared<v2::NodeServiceImpl>(v2::NodeServiceImplContext{
          .executor_ = executor_,
          .view_service_ = *server_address_space_,
          .attribute_service_ = *server_address_space_,
          .monitored_item_service_ = monitored_item_service_,
          .view_events_provider_ = base_env_.view_events_provider});
};

TEST_F(V2NodeServiceRegressionTest,
       FetchNodeAndChildren_PreservesChildrenFetchedWhenChildrenWinRace) {
  OpenChannel();

  const auto node_id = server_address_space_->kTestNode2Id;

  EXPECT_CALL(*server_address_space_, Read(_, _)).Times(AnyNumber());
  EXPECT_CALL(*server_address_space_, Browse(_, _)).Times(AnyNumber());

  auto node = node_service_->GetNode(node_id);

  node.StartFetch(NodeFetchStatus::NodeAndChildren());

  DrainExecutor();

  DrainExecutor();

  EXPECT_TRUE(node.fetched());
  EXPECT_TRUE(node.children_fetched());
  EXPECT_TRUE(node.status());
  EXPECT_EQ(node.target(server_address_space_->kTestReferenceTypeId).node_id(),
            server_address_space_->kTestNode3Id);
}
