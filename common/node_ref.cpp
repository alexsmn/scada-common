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
  return model_ ? model_->GetTarget(scada::id::HasTypeDefinition, true)
                : nullptr;
}

NodeRef NodeRef::supertype() const {
  return model_ ? model_->GetTarget(scada::id::HasSubtype, false) : nullptr;
}

NodeRef NodeRef::parent() const {
  return model_ ? model_->GetTarget(scada::id::HierarchicalReferences, false)
                : nullptr;
}

std::vector<NodeRef> NodeRef::aggregates() const {
  return model_ ? model_->GetTargets(scada::id::Aggregates, true)
                : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::components() const {
  return model_ ? model_->GetTargets(scada::id::HasComponent, true)
                : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::properties() const {
  return model_ ? model_->GetTargets(scada::id::HasProperty, true)
                : std::vector<NodeRef>{};
}

std::vector<NodeRef::Reference> NodeRef::references() const {
  return model_ ? model_->GetReferences(scada::id::References, true)
                : std::vector<Reference>{};
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTarget(reference_type_id, true) : NodeRef{};
}

std::vector<NodeRef> NodeRef::targets(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTargets(reference_type_id, true)
                : std::vector<NodeRef>();
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

NodeRef NodeRef::operator[](
    const scada::NodeId& aggregate_declaration_id) const {
  if (!model_)
    return nullptr;

  auto aggregate_declaration =
      model_->GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;

  return model_->GetAggregate(aggregate_declaration.browse_name());
}

NodeRef NodeRef::operator[](const scada::QualifiedName& aggregate_name) const {
  return model_ ? model_->GetAggregate(aggregate_name) : NodeRef{};
}

NodeRef NodeRef::GetAggregateDeclaration(
    const scada::NodeId& aggregate_declaration_id) const {
  return model_ ? model_->GetAggregateDeclaration(aggregate_declaration_id)
                : nullptr;
}

void NodeRef::Fetch(const NodeFetchStatus& requested_status,
                    const FetchCallback& callback) const {
  assert(!requested_status.empty());

  if (model_) {
    model_->Fetch(requested_status,
                  [copy = *this, callback] { callback(copy); });
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
