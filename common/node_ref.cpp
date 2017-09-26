#include "common/node_ref.h"

#include "common/node_ref_impl.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"

NodeRef::NodeRef(std::shared_ptr<NodeRefImpl> impl)
    : impl_{std::move(impl)} {
}

scada::NodeId NodeRef::id() const {
  return GetAttribute(OpcUa_Attributes_NodeId).get_or(scada::NodeId{});
}

std::optional<scada::NodeClass> NodeRef::node_class() const {
  auto value = GetAttribute(OpcUa_Attributes_NodeClass);
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

scada::QualifiedName NodeRef::browse_name() const {
  return GetAttribute(OpcUa_Attributes_BrowseName).get_or(scada::QualifiedName{});
}

scada::LocalizedText NodeRef::display_name() const {
  return GetAttribute(OpcUa_Attributes_DisplayName).get_or(scada::LocalizedText{});
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

NodeRef NodeRef::operator[](const scada::QualifiedName& aggregate_name) const {
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
  return impl_ ? impl_->GetValue() : scada::DataValue{};
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  return impl_ ? impl_->GetTarget(reference_type_id) : NodeRef{};
}

std::vector<NodeRef> NodeRef::targets(const scada::NodeId& reference_type_id) const {
  return impl_ ? impl_->GetTargets(reference_type_id) : std::vector<NodeRef>();
}

scada::Variant NodeRef::GetAttribute(scada::AttributeId attribute_id) const {
  return impl_ ? impl_->GetAttribute(attribute_id) : scada::Variant{};
}

NodeRef NodeRef::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  return impl_ ? impl_->GetAggregateDeclaration(aggregate_declaration_id) : nullptr;
}

void NodeRef::Fetch(const FetchCallback& callback) {
  if (impl_)
    impl_->Fetch(callback);
  else
    callback(*this);
}

scada::Status NodeRef::status() const {
  return impl_ ? impl_->GetStatus() : scada::StatusCode::Good;
}

void NodeRef::Browse(const scada::BrowseDescription& description, const BrowseCallback& callback) const {
  if (impl_)
    impl_->Browse(description, callback);
  else
    callback(scada::StatusCode::Bad, {});
}

void NodeRef::AddObserver(NodeRefObserver& observer) {
  if (impl_)
    impl_->AddObserver(observer);
}

void NodeRef::RemoveObserver(NodeRefObserver& observer) {
  if (impl_)
    impl_->RemoveObserver(observer);
}
