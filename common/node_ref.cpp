#include "common/node_ref.h"

#include "common/node_ref_impl.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"

NodeRef::NodeRef(std::shared_ptr<NodeRefImpl> impl)
    : impl_{std::move(impl)} {
}

scada::NodeId NodeRef::id() const {
  return GetAttribute(OpcUa_Attributes_NodeId).value.get_or(scada::NodeId{});
}

base::Optional<scada::NodeClass> NodeRef::node_class() const {
  auto value = GetAttribute(OpcUa_Attributes_NodeClass).value;
  if (value.is_null())
    return {};
  return static_cast<scada::NodeClass>(value.get<int>());
}

bool NodeRef::fetched() const {
  return !impl_ || impl_->IsFetched();
}

NodeRef NodeRef::data_type() const {
  return impl_ ? impl_->GetDataType() : nullptr;
}

std::string NodeRef::browse_name() const {
  return GetAttribute(OpcUa_Attributes_BrowseName).value.get_or(std::string{});
}

base::string16 NodeRef::display_name() const {
  return GetAttribute(OpcUa_Attributes_DisplayName).value.get_or(base::string16{});
}

NodeRef NodeRef::type_definition() const {
  return impl_ ? impl_->GetTypeDefinition() : nullptr;
}

NodeRef NodeRef::supertype() const {
  return impl_ ? impl_->GetSupertype() : nullptr;
}

std::vector<NodeRef> NodeRef::aggregates() const {
  return impl_ ? impl_->GetAggregates(OpcUaId_Aggregates) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::components() const {
  return impl_ ? impl_->GetAggregates(OpcUaId_HasComponent) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::properties() const {
  return impl_ ? impl_->GetAggregates(OpcUaId_HasProperty) : std::vector<NodeRef>{};
}

std::vector<NodeRef::Reference> NodeRef::references() const {
  return impl_ ? impl_->GetReferences() : std::vector<Reference>{};
}

NodeRef NodeRef::operator[](base::StringPiece aggregate_name) const {
  return impl_ ? impl_->GetAggregate(aggregate_name) : NodeRef{};
}

NodeRef NodeRef::operator[](const scada::NodeId& aggregate_declaration_id) const {
  if (!impl_)
    return nullptr;

  auto aggregate_declaration = impl_->GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;

  return impl_->GetAggregate(aggregate_declaration.browse_name());
}

scada::DataValue NodeRef::data_value() const {
  return GetAttribute(OpcUa_Attributes_Value);
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return impl_ ? impl_->GetTarget(reference_type_id) : NodeRef{};
}

std::vector<NodeRef> NodeRef::targets(const scada::NodeId& reference_type_id) const {
  return impl_ ? impl_->GetTargets(reference_type_id) : std::vector<NodeRef>();
}

scada::DataValue NodeRef::GetAttribute(scada::AttributeId attribute_id) const {
  return impl_ ? impl_->GetAttribute(attribute_id) : scada::DataValue{};
}

NodeRef NodeRef::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  return impl_ ? impl_->GetAggregateDeclaration(aggregate_declaration_id) : nullptr;
}

void NodeRef::Fetch(const FetchCallback& callback) {
  impl_->Fetch(callback);
}

scada::Status NodeRef::status() const {
  return impl_ ? impl_->GetStatus() : scada::StatusCode::Good;
}
