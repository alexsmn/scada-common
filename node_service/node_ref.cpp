#include "node_service/node_ref.h"

#include "node_service/node_model.h"

bool NodeRef::fetched() const {
  return !model_ || model_->GetFetchStatus().node_fetched;
}

bool NodeRef::children_fetched() const {
  return !model_ || model_->GetFetchStatus().children_fetched;
}

std::optional<scada::NodeClass> NodeRef::node_class() const {
  const auto& value = attribute(scada::AttributeId::NodeClass);
  if (auto* int_value = value.get_if<int>()) {
    return static_cast<scada::NodeClass>(*int_value);
  } else {
    return std::nullopt;
  }
}

scada::NodeId NodeRef::node_id() const {
  return attribute(scada::AttributeId::NodeId).get_or(scada::NodeId{});
}

scada::QualifiedName NodeRef::browse_name() const {
  return attribute(scada::AttributeId::BrowseName)
      .get_or(scada::QualifiedName{});
}

scada::LocalizedText NodeRef::display_name() const {
  return attribute(scada::AttributeId::DisplayName)
      .get_or(scada::LocalizedText{});
}

scada::Variant NodeRef::value() const {
  return attribute(scada::AttributeId::Value);
}

NodeRef NodeRef::type_definition() const {
  return target(scada::id::HasTypeDefinition);
}

NodeRef NodeRef::supertype() const {
  return inverse_target(scada::id::HasSubtype);
}

NodeRef NodeRef::parent() const {
  return inverse_target(scada::id::HierarchicalReferences);
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTarget(reference_type_id, true) : nullptr;
}

NodeRef NodeRef::inverse_target(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTarget(reference_type_id, false) : nullptr;
}

bool NodeRef::has_target(const scada::NodeId& reference_type_id,
                         bool forward,
                         const scada::NodeId& node_id) const {
  return !!reference(reference_type_id, forward, node_id).target;
}

std::vector<NodeRef> NodeRef::targets(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTargets(reference_type_id, true)
                : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::inverse_targets(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTargets(reference_type_id, false)
                : std::vector<NodeRef>{};
}

scada::Variant NodeRef::attribute(scada::AttributeId attribute_id) const {
  return model_ ? model_->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef NodeRef::data_type() const {
  return model_ ? model_->GetDataType() : nullptr;
}

NodeRef::Reference NodeRef::reference(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetReference(reference_type_id, true, {})
                : NodeRef::Reference{};
}

NodeRef::Reference NodeRef::inverse_reference(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetReference(reference_type_id, false, {})
                : NodeRef::Reference{};
}

NodeRef::Reference NodeRef::reference(const scada::NodeId& reference_type_id,
                                      bool forward,
                                      const scada::NodeId& node_id) const {
  return model_ ? model_->GetReference(reference_type_id, forward, node_id)
                : NodeRef::Reference{};
}

std::vector<NodeRef::Reference> NodeRef::references(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetReferences(reference_type_id, true)
                : std::vector<NodeRef::Reference>{};
}

std::vector<NodeRef::Reference> NodeRef::inverse_references(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetReferences(reference_type_id, false)
                : std::vector<NodeRef::Reference>{};
}

NodeRef NodeRef::operator[](
    const scada::NodeId& aggregate_declaration_id) const {
  return model_ ? model_->GetAggregate(aggregate_declaration_id) : nullptr;
}

NodeRef NodeRef::operator[](const scada::QualifiedName& child_name) const {
  return model_ ? model_->GetChild(child_name) : NodeRef{};
}

void NodeRef::Fetch(const NodeFetchStatus& requested_status) const {
  assert(!requested_status.empty());

  if (model_)
    model_->Fetch(requested_status, {});
}

void NodeRef::Fetch(const NodeFetchStatus& requested_status,
                    const FetchCallback& callback) const {
  assert(!requested_status.empty());

  if (model_) {
    if (callback) {
      model_->Fetch(requested_status,
                    [copy = *this, callback] { callback(copy); });
    } else {
      model_->Fetch(requested_status, nullptr);
    }

  } else {
    if (callback)
      callback(*this);
  }
}

scada::Status NodeRef::status() const {
  return model_ ? model_->GetStatus() : scada::StatusCode::Bad_WrongNodeId;
}

void NodeRef::Subscribe(NodeRefObserver& observer) const {
  if (model_)
    model_->Subscribe(observer);
}

void NodeRef::Unsubscribe(NodeRefObserver& observer) const {
  if (model_)
    model_->Unsubscribe(observer);
}

scada::node NodeRef::scada_node() const {
  return model_ ? model_->GetScadaNode() : scada::node{};
}