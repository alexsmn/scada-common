#include "common/node_ref.h"

#include "common/node_model.h"
#include "common/node_util.h"
#include "core/standard_node_ids.h"

bool NodeRef::fetched() const {
  return fetch_status().node_fetched;
}

std::optional<scada::NodeClass> NodeRef::node_class() const {
  if (auto* int_value = attribute(scada::AttributeId::NodeClass).get_if<int>())
    return static_cast<scada::NodeClass>(*int_value);
  else
    return std::nullopt;
}

scada::NodeId NodeRef::id() const {
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
  return target(scada::id::HasSubtype, false);
}

NodeRef NodeRef::parent() const {
  return target(scada::id::HierarchicalReferences, false);
}

std::vector<NodeRef> NodeRef::children() const {
  return targets(scada::id::HasChild, true);
}

std::vector<NodeRef> NodeRef::aggregates() const {
  return targets(scada::id::Aggregates, true);
}

std::vector<NodeRef> NodeRef::components() const {
  return targets(scada::id::HasComponent, true);
}

std::vector<NodeRef> NodeRef::organizes() const {
  return targets(scada::id::Organizes, true);
}

std::vector<NodeRef> NodeRef::properties() const {
  return targets(scada::id::HasProperty, true);
}

std::vector<NodeRef::Reference> NodeRef::references() const {
  return references(scada::id::NonHierarchicalReferences, true);
}

std::vector<NodeRef::Reference> NodeRef::inverse_references(
    const scada::NodeId& reference_type_id) const {
  return references(reference_type_id, false);
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTarget(reference_type_id, true) : NodeRef{};
}

std::vector<NodeRef> NodeRef::targets(
    const scada::NodeId& reference_type_id) const {
  return targets(reference_type_id, true);
}

NodeFetchStatus NodeRef::fetch_status() const {
  return model_ ? model_->GetFetchStatus() : NodeFetchStatus::Max();
}

scada::Variant NodeRef::attribute(scada::AttributeId attribute_id) const {
  return model_ ? model_->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef NodeRef::data_type() const {
  return model_ ? model_->GetDataType() : nullptr;
}

NodeRef::Reference NodeRef::reference(const scada::NodeId& reference_type_id,
                                      bool forward) const {
  return model_ ? model_->GetReference(reference_type_id, forward)
                : NodeRef::Reference{};
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id,
                        bool forward) const {
  return model_ ? model_->GetTarget(reference_type_id, forward) : nullptr;
}

std::vector<NodeRef> NodeRef::targets(const scada::NodeId& reference_type_id,
                                      bool forward) const {
  return model_ ? model_->GetTargets(reference_type_id, forward)
                : std::vector<NodeRef>{};
}

std::vector<NodeRef::Reference> NodeRef::references(
    const scada::NodeId& reference_type_id,
    bool forward) const {
  return model_ ? model_->GetReferences(reference_type_id, forward)
                : std::vector<NodeRef::Reference>{};
}

NodeRef NodeRef::operator[](
    const scada::NodeId& aggregate_declaration_id) const {
  return model_ ? model_->GetAggregate(aggregate_declaration_id) : nullptr;
}

NodeRef NodeRef::operator[](const scada::QualifiedName& aggregate_name) const {
  return model_ ? model_->GetAggregate(aggregate_name) : NodeRef{};
}

void NodeRef::Fetch(const NodeFetchStatus& requested_status,
                    const FetchCallback& callback) const {
  assert(!requested_status.empty());

  if (model_) {
    if (callback) {
      model_->Fetch(requested_status,
                    [copy = *this, callback] { callback(copy); });
    } else {
      model_->Fetch(requested_status, {});
    }

  } else {
    callback(*this);
  }
}

scada::Status NodeRef::status() const {
  return model_ ? model_->GetStatus() : scada::StatusCode::Good;
}

void NodeRef::Subscribe(NodeRefObserver& observer) const {
  if (model_)
    model_->Subscribe(observer);
}

void NodeRef::Unsubscribe(NodeRefObserver& observer) const {
  if (model_)
    model_->Unsubscribe(observer);
}
