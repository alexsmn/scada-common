#include "common/node_ref.h"

#include "common/node_model.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"

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
  return attribute(scada::AttributeId::BrowseName).get_or(scada::QualifiedName{});
}

scada::LocalizedText NodeRef::display_name() const {
  return attribute(scada::AttributeId::DisplayName).get_or(scada::LocalizedText{});
}

NodeRef NodeRef::type_definition() const {
  return model_ ? model_->GetTypeDefinition() : nullptr;
}

NodeRef NodeRef::supertype() const {
  return model_ ? model_->GetSupertype() : nullptr;
}

std::vector<NodeRef> NodeRef::aggregates() const {
  return model_ ? model_->GetAggregates(scada::id::Aggregates) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::components() const {
  return model_ ? model_->GetAggregates(scada::id::HasComponent) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::properties() const {
  return model_ ? model_->GetAggregates(scada::id::HasProperty) : std::vector<NodeRef>{};
}

std::vector<NodeRef::Reference> NodeRef::references() const {
  return model_ ? model_->GetReferences() : std::vector<Reference>{};
}

NodeRef NodeRef::operator[](const scada::QualifiedName& aggregate_name) const {
  return model_ ? model_->GetAggregate(aggregate_name) : NodeRef{};
}

NodeRef NodeRef::operator[](const scada::NodeId& aggregate_declaration_id) const {
  if (!model_)
    return nullptr;

  auto aggregate_declaration = model_->GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;

  return model_->GetAggregate(aggregate_declaration.browse_name());
}

scada::Variant NodeRef::value() const {
  return model_ ? model_->GetValue() : scada::Variant{};
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTarget(reference_type_id) : NodeRef{};
}

std::vector<NodeRef> NodeRef::targets(const scada::NodeId& reference_type_id) const {
  return model_ ? model_->GetTargets(reference_type_id) : std::vector<NodeRef>();
}

scada::Variant NodeRef::attribute(scada::AttributeId attribute_id) const {
  return model_ ? model_->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef NodeRef::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  return model_ ? model_->GetAggregateDeclaration(aggregate_declaration_id) : nullptr;
}

void NodeRef::Fetch(const FetchCallback& callback) {
  if (model_)
    model_->Fetch(callback);
  else
    callback(*this);
}

scada::Status NodeRef::status() const {
  return model_ ? model_->GetStatus() : scada::StatusCode::Good;
}

void NodeRef::Browse(const scada::BrowseDescription& description, const BrowseCallback& callback) const {
  if (model_)
    model_->Browse(description, callback);
  else
    callback(scada::StatusCode::Bad, {});
}

void NodeRef::AddObserver(NodeRefObserver& observer) {
  if (model_)
    model_->AddObserver(observer);
}

void NodeRef::RemoveObserver(NodeRefObserver& observer) {
  if (model_)
    model_->RemoveObserver(observer);
}