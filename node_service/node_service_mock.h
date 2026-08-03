#pragma once

#include "node_service/node_service.h"

#include <gmock/gmock.h>

class MockNodeService : public NodeService {
 public:
  MOCK_METHOD(NodeRef, GetNode, (const scada::NodeId& node_id), (override));

  // --- Per-node operations. ---

  MOCK_METHOD(scada::Status,
              GetStatus,
              (const scada::NodeId& node_id),
              (override));
  MOCK_METHOD(NodeFetchStatus,
              GetFetchStatus,
              (const scada::NodeId& node_id),
              (override));

  MOCK_METHOD(Awaitable<void>,
              Fetch,
              (const scada::NodeId& node_id,
               const NodeFetchStatus& requested_status),
              (override));
  MOCK_METHOD(void,
              StartFetch,
              (const scada::NodeId& node_id,
               const NodeFetchStatus& requested_status),
              (override));

  MOCK_METHOD(scada::Variant,
              GetAttribute,
              (const scada::NodeId& node_id, scada::AttributeId attribute_id),
              (override));

  MOCK_METHOD(NodeRef, GetDataType, (const scada::NodeId& node_id), (override));

  MOCK_METHOD(NodeRef::Reference,
              GetReference,
              (const scada::NodeId& node_id,
               const scada::NodeId& reference_type_id,
               bool forward,
               const scada::NodeId& target_id),
              (override));
  MOCK_METHOD(std::vector<NodeRef::Reference>,
              GetReferences,
              (const scada::NodeId& node_id,
               const scada::NodeId& reference_type_id,
               bool forward),
              (override));

  MOCK_METHOD(NodeRef,
              GetTarget,
              (const scada::NodeId& node_id,
               const scada::NodeId& reference_type_id,
               bool forward),
              (override));
  MOCK_METHOD(std::vector<NodeRef>,
              GetTargets,
              (const scada::NodeId& node_id,
               const scada::NodeId& reference_type_id,
               bool forward),
              (override));

  MOCK_METHOD(NodeRef,
              GetAggregate,
              (const scada::NodeId& node_id,
               const scada::NodeId& aggregate_declaration_id),
              (override));
  MOCK_METHOD(NodeRef,
              GetChild,
              (const scada::NodeId& node_id,
               const scada::QualifiedName& child_name),
              (override));

  MOCK_METHOD(scada::node,
              GetScadaNode,
              (const scada::NodeId& node_id),
              (override));

  // --- Node-scoped subscriptions. ---

  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeModelChanged,
              (const scada::NodeId& node_id,
               const ModelChangedCallback& callback),
              (override));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeSemanticChanged,
              (const scada::NodeId& node_id,
               const NodeSemanticChangedCallback& callback),
              (override));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeFetched,
              (const scada::NodeId& node_id,
               const NodeFetchedCallback& callback),
              (override));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeStateChanged,
              (const scada::NodeId& node_id,
               const NodeStateChangedCallback& callback),
              (override));

  // --- Service-wide subscriptions (all nodes). ---

  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeModelChanged,
              (const ModelChangedCallback& callback),
              (const, override));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeSemanticChanged,
              (const NodeSemanticChangedCallback& callback),
              (const, override));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeFetched,
              (const NodeFetchedCallback& callback),
              (const, override));
  MOCK_METHOD(boost::signals2::scoped_connection,
              SubscribeNodeStateChanged,
              (const NodeStateChangedCallback& callback),
              (const, override));

  MOCK_METHOD(size_t, GetPendingTaskCount, (), (const, override));
};
