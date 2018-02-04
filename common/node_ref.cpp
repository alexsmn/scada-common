#include "common/node_ref.h"

#include "common/node_model.h"
#include "common/node_ref_util.h"
#include "common/node_service.h"

scada::NodeId NodeRef::id() const {
  return attribute(scada::AttributeId::NodeId).get_or(scada::NodeId{});
}

std::optional<scada::NodeClass> NodeRef::node_class() const {
  auto value = attribute(scada::AttributeId::NodeClass);
  if (value.is_null())
    return {};
  return static_cast<scada::NodeClass>(value.get<int>());
}

bool NodeRef::fetched() const {
  return !model_ || model_->IsFetched();
}

NodeRef NodeRef::data_type() const {
  return model_ ? model_->GetDataType() : nullptr;
}

scada::QualifiedName NodeRef::browse_name() const {
  return attribute(scada::AttributeId::BrowseName)
      .get_or(scada::QualifiedName{});
}

scada::LocalizedText NodeRef::display_name() const {
  return attribute(scada::AttributeId::DisplayName)
      .get_or(scada::LocalizedText{});
}

NodeRef NodeRef::type_definition() const {
  return model_ ? model_->GetTarget(scada::id::HasTypeDefinition, true)
                : nullptr;
}

NodeRef NodeRef::supertype() const {
  return model_ ? model_->GetTarget(scada::id::HasSubtype, false) : nullptr;
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

NodeRef NodeRef::operator[](const scada::QualifiedName& aggregate_name) const {
  return model_ ? model_->GetAggregate(aggregate_name) : NodeRef{};
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

scada::Variant NodeRef::value() const {
  return attribute(scada::AttributeId::Value);
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTarget(reference_type_id, true) : NodeRef{};
}

std::vector<NodeRef> NodeRef::targets(
    const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTargets(reference_type_id, true)
                : std::vector<NodeRef>();
}

scada::Variant NodeRef::attribute(scada::AttributeId attribute_id) const {
  return model_ ? model_->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef NodeRef::GetAggregateDeclaration(
    const scada::NodeId& aggregate_declaration_id) const {
  return model_ ? model_->GetAggregateDeclaration(aggregate_declaration_id)
                : nullptr;
}

void NodeRef::Fetch(const FetchCallback& callback) const {
  if (model_)
    model_->Fetch(callback);
  else
    callback(*this);
}

scada::Status NodeRef::status() const {
  return model_ ? model_->GetStatus() : scada::StatusCode::Good;
}

void NodeRef::Subscribe(NodeRefObserver& observer) {
  if (model_)
    model_->Subscribe(observer);
}

void NodeRef::Unsubscribe(NodeRefObserver& observer) {
  if (model_)
    model_->Unsubscribe(observer);
}

NodeRef NodeRef::parent() const {
  return model_ ? model_->GetTarget(scada::id::HierarchicalReferences, false)
                : nullptr;
}
