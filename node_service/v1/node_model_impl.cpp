#include "node_service/v1/node_model_impl.h"

#include "core/attribute_service.h"
#include "core/method_service.h"
#include "core/monitored_item_service.h"
#include "model/node_id_util.h"
#include "node_service/node_observer.h"
#include "node_service/node_service.h"

namespace v1 {

NodeModelImpl::NodeModelImpl(AddressSpaceNodeModelContext&& context)
    : AddressSpaceNodeModelContext{std::move(context)} {}

NodeModelImpl::~NodeModelImpl() {
  assert(!observers_.might_have_observers());

  // delegate_.OnNodeModelDeleted(node_id_);
}

void NodeModelImpl::OnModelChanged(const scada::ModelChangeEvent& event) {
  // The node reference must be cleaned up at first, as the observers may access
  // to the object.
  if (event.verb & scada::ModelChangeEvent::NodeDeleted)
    OnNodeDeleted();

  for (auto& o : observers_)
    o.OnModelChanged(event);
}

void NodeModelImpl::OnNodeSemanticChanged() {
  for (auto& o : observers_)
    o.OnNodeSemanticChanged(node_id_);
}

scada::Variant NodeModelImpl::GetAttribute(
    scada::AttributeId attribute_id) const {
  if (attribute_id == scada::AttributeId::NodeId)
    return node_id_;

  if (!node_) {
    switch (attribute_id) {
      case scada::AttributeId::DisplayName:
        return scada::ToLocalizedText(NodeIdToScadaString(node_id_));
    }
    return {};
  }

  switch (attribute_id) {
    case scada::AttributeId::NodeClass:
      return static_cast<int>(node_->GetNodeClass());
    case scada::AttributeId::BrowseName:
      return node_->GetBrowseName();
    case scada::AttributeId::DisplayName:
      return node_->GetDisplayName();
  }

  if (auto* variable = scada::AsVariable(node_)) {
    switch (attribute_id) {
      case scada::AttributeId::Value:
        return variable->GetValue().value;
    }
  }

  return {};
}

std::vector<NodeRef::Reference> NodeModelImpl::GetReferences(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef::Reference> result;

  if (!node_)
    return result;

  auto& refs =
      forward ? node_->forward_references() : node_->inverse_references();
  for (auto& r : refs) {
    if (scada::IsSubtypeOf(*r.type, reference_type_id))
      result.push_back({delegate_.GetRemoteNode(r.type),
                        delegate_.GetRemoteNode(r.node), forward});
  }

  return result;
}

NodeRef NodeModelImpl::GetDataType() const {
  auto* variable = scada::AsVariable(node_);
  auto* data_type = variable ? &variable->GetDataType() : nullptr;
  return delegate_.GetRemoteNode(data_type);
}

NodeRef NodeModelImpl::GetAggregate(const scada::NodeId& declaration_id) const {
  if (!node_)
    return nullptr;

  auto* type_definition = scada::IsTypeDefinition(node_->GetNodeClass())
                              ? scada::AsTypeDefinition(node_)
                              : node_->type_definition();
  if (!type_definition)
    return nullptr;

  auto* declaration =
      scada::GetAggregateDeclaration(*type_definition, declaration_id);
  if (!declaration)
    return nullptr;

  if (scada::IsTypeDefinition(node_->GetNodeClass()))
    return delegate_.GetRemoteNode(declaration);
  else
    return GetChild(declaration->GetBrowseName());
}

NodeRef NodeModelImpl::GetChild(const scada::QualifiedName& child_name) const {
  assert(fetch_status_.node_fetched);

  if (!node_)
    return nullptr;

  for (auto* node : scada::GetChildren(*node_)) {
    if (node->GetBrowseName() == child_name)
      return delegate_.GetRemoteNode(node);
  }

  return nullptr;
}

NodeRef NodeModelImpl::GetTarget(const scada::NodeId& reference_type_id,
                                 bool forward) const {
  if (!node_)
    return nullptr;

  auto& refs =
      forward ? node_->forward_references() : node_->inverse_references();
  for (auto& r : refs) {
    if (scada::IsSubtypeOf(*r.type, reference_type_id))
      return delegate_.GetRemoteNode(r.node);
  }

  return nullptr;
}

std::vector<NodeRef> NodeModelImpl::GetTargets(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  std::vector<NodeRef> result;

  if (!node_)
    return result;

  auto& refs =
      forward ? node_->forward_references() : node_->inverse_references();
  for (auto& r : refs) {
    if (scada::IsSubtypeOf(*r.type, reference_type_id))
      result.emplace_back(delegate_.GetRemoteNode(r.node));
  }

  return result;
}

NodeRef::Reference NodeModelImpl::GetReference(
    const scada::NodeId& reference_type_id,
    bool forward,
    const scada::NodeId& node_id) const {
  if (!node_)
    return {};

  auto reference = scada::FindReference(*node_, reference_type_id, forward);
  return {delegate_.GetRemoteNode(reference.type),
          delegate_.GetRemoteNode(reference.node), forward};
}

void NodeModelImpl::SetFetchStatus(const scada::Node* node,
                                   const scada::Status& status,
                                   const NodeFetchStatus& fetch_status) {
  node_ = node;

  BaseNodeModel::SetFetchStatus(status, fetch_status);
}

void NodeModelImpl::NotifyFetchStatus() {
  const scada::ModelChangeEvent event{
      node_id_, node_ ? scada::GetTypeDefinitionId(*node_) : scada::NodeId{},
      scada::ModelChangeEvent::ReferenceAdded |
          scada::ModelChangeEvent::ReferenceDeleted};
  for (auto& o : observers_) {
    o.OnModelChanged(event);
    o.OnNodeSemanticChanged(node_id_);
    o.OnNodeFetched(node_id_, fetch_status_.children_fetched);
  }
}

void NodeModelImpl::OnFetchRequested(const NodeFetchStatus& requested_status) {
  delegate_.OnNodeModelFetchRequested(node_id_, requested_status);
}

void NodeModelImpl::OnNodeDeleted() {
  node_ = nullptr;

  BaseNodeModel::OnNodeDeleted();
}

std::shared_ptr<scada::MonitoredItem> NodeModelImpl::CreateMonitoredItem(
    scada::AttributeId attribute_id,
    const scada::MonitoringParameters& params) const {
  return monitored_item_service_.CreateMonitoredItem({node_id_, attribute_id},
                                                     params);
}

void NodeModelImpl::Read(scada::AttributeId attribute_id,
                         const NodeRef::ReadCallback& callback) const {
  std::vector<scada::ReadValueId> inputs;
  inputs.emplace_back(scada::ReadValueId{node_id_, attribute_id});

  attribute_service_.Read(
      inputs, [callback](scada::Status&& status,
                         std::vector<scada::DataValue>&& values) {
        if (!status)
          return callback(scada::MakeReadError(status.code()));

        // Must be guaranteed above.
        assert(values.size() == 1);
        auto& value = values.front();
        callback(std::move(value));
      });
}

void NodeModelImpl::Write(scada::AttributeId attribute_id,
                          const scada::Variant& value,
                          const scada::WriteFlags& flags,
                          const scada::NodeId& user_id,
                          const scada::StatusCallback& callback) const {
  std::vector<scada::WriteValue> inputs;
  inputs.emplace_back(
      scada::WriteValue{node_id_, attribute_id, value, flags});

  attribute_service_.Write(
      inputs, user_id,
      [callback](scada::Status&& status,
                 std::vector<scada::StatusCode>&& results) {
        callback(status ? std::move(results.front()) : std::move(status));
      });
}

void NodeModelImpl::Call(const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments,
                         const scada::NodeId& user_id,
                         const scada::StatusCallback& callback) const {
  method_service_.Call(node_id_, method_id, arguments, user_id, callback);
}

}  // namespace v1
