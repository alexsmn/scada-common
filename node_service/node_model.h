#pragma once

#include "node_service/node_ref.h"
#include "core/configuration_types.h"

#include <functional>
#include <vector>

class NodeRefObserver;

class NodeModel {
 public:
  virtual ~NodeModel() {}

  virtual scada::Status GetStatus() const = 0;

  virtual NodeFetchStatus GetFetchStatus() const = 0;

  using FetchCallback = std::function<void()>;
  virtual void Fetch(const NodeFetchStatus& requested_status,
                     const FetchCallback& callback) const = 0;

  virtual scada::Variant GetAttribute(
      scada::AttributeId attribute_id) const = 0;

  virtual NodeRef GetDataType() const = 0;

  virtual NodeRef::Reference GetReference(
      const scada::NodeId& reference_type_id,
      bool forward,
      const scada::NodeId& node_id) const = 0;
  virtual std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& reference_type_id,
      bool forward) const = 0;

  virtual NodeRef GetTarget(const scada::NodeId& reference_type_id,
                            bool forward) const = 0;
  virtual std::vector<NodeRef> GetTargets(
      const scada::NodeId& reference_type_id,
      bool forward) const = 0;

  virtual NodeRef GetAggregate(
      const scada::NodeId& aggregate_declaration_id) const = 0;
  virtual NodeRef GetChild(const scada::QualifiedName& child_name) const = 0;

  virtual void Subscribe(NodeRefObserver& observer) const = 0;
  virtual void Unsubscribe(NodeRefObserver& observer) const = 0;

  virtual std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id,
      const scada::MonitoringParameters& params) const = 0;

  virtual void Read(scada::AttributeId attribute_id,
                    const NodeRef::ReadCallback& callback) const = 0;

  virtual void Write(scada::AttributeId attribute_id,
                     const scada::Variant& value,
                     const scada::WriteFlags& flags,
                     const scada::NodeId& user_id,
                     const scada::StatusCallback& callback) const = 0;

  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const scada::StatusCallback& callback) const = 0;
};
