#pragma once

#include "node_service/node_model.h"

#include <gmock/gmock.h>

class MockNodeModel : public NodeModel {
 public:
  MOCK_METHOD(scada::Status, GetStatus, (), (const));

  MOCK_METHOD(NodeFetchStatus, GetFetchStatus, (), (const));

  MOCK_METHOD(void,
              Fetch,
              (const NodeFetchStatus& requested_status,
               const FetchCallback& callback),
              (const));

  MOCK_METHOD(scada::Variant,
              GetAttribute,
              (scada::AttributeId attribute_id),
              (const));

  MOCK_METHOD(NodeRef, GetDataType, (), (const));

  MOCK_METHOD(NodeRef::Reference,
              GetReference,
              (const scada::NodeId& reference_type_id,
               bool forward,
               const scada::NodeId& node_id),
              (const));
  MOCK_METHOD(std::vector<NodeRef::Reference>,
              GetReferences,
              (const scada::NodeId& reference_type_id, bool forward),
              (const));

  MOCK_METHOD(NodeRef,
              GetTarget,
              (const scada::NodeId& reference_type_id, bool forward),
              (const));
  MOCK_METHOD(std::vector<NodeRef>,
              GetTargets,
              (const scada::NodeId& reference_type_id, bool forward),
              (const));

  MOCK_METHOD(NodeRef,
              GetAggregate,
              (const scada::NodeId& aggregate_declaration_id),
              (const));
  MOCK_METHOD(NodeRef,
              GetChild,
              (const scada::QualifiedName& child_name),
              (const));

  MOCK_METHOD(void, Subscribe, (NodeRefObserver & observer), (const));
  MOCK_METHOD(void, Unsubscribe, (NodeRefObserver & observer), (const));

  MOCK_METHOD(std::shared_ptr<scada::MonitoredItem>,
              CreateMonitoredItem,
              (scada::AttributeId attribute_id,
               const scada::MonitoringParameters& params),
              (const));

  MOCK_METHOD(void,
              Read,
              (scada::AttributeId attribute_id,
               const NodeRef::ReadCallback& callback),
              (const));

  MOCK_METHOD(void,
              Write,
              (scada::AttributeId attribute_id,
               const scada::Variant& value,
               const scada::WriteFlags& flags,
               const scada::NodeId& user_id,
               const scada::StatusCallback& callback),
              (const));

  MOCK_METHOD(void,
              Call,
              (const scada::NodeId& method_id,
               const std::vector<scada::Variant>& arguments,
               const scada::NodeId& user_id,
               const scada::StatusCallback& callback),
              (const));

  MOCK_METHOD(scada::node, GetScadaNode, (), (const));
};
