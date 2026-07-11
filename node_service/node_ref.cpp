#include "node_service/node_ref.h"

#include "node_service/node_service.h"

#if defined(SCADA_USE_CORE_MODULE)
// Modules-pilot consumer (SCADA_CXX_MODULES=ON): base names come from the
// scada.core facade. The import sits after the textual includes because the
// reverse order trips an AppleClang 21 declaration-merging bug in libc++.
import scada.core;
#else
#include "base/check.h"
#endif

scada::Status NodeRef::status() const {
  return service_ ? service_->GetStatus(id_)
                  : scada::Status{scada::StatusCode::Bad_WrongNodeId};
}

bool NodeRef::fetched() const {
  return !service_ || service_->GetFetchStatus(id_).node_fetched;
}

bool NodeRef::children_fetched() const {
  return !service_ || service_->GetFetchStatus(id_).children_fetched;
}

Awaitable<NodeRef> NodeRef::Fetch(
    const NodeFetchStatus& requested_status) const {
  base::Check(!requested_status.empty());

  if (service_)
    co_await service_->Fetch(id_, requested_status);

  co_return *this;
}

void NodeRef::StartFetch(const NodeFetchStatus& requested_status) const {
  base::Check(!requested_status.empty());

  if (service_)
    service_->StartFetch(id_, requested_status);
}

scada::Variant NodeRef::attribute(scada::AttributeId attribute_id) const {
  return service_ ? service_->GetAttribute(id_, attribute_id)
                  : scada::Variant{};
}

std::optional<scada::NodeClass> NodeRef::node_class() const {
  const auto& value = attribute(scada::AttributeId::NodeClass);
  if (auto* int_value = value.get_if<int>()) {
    return static_cast<scada::NodeClass>(*int_value);
  } else {
    return std::nullopt;
  }
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

NodeRef NodeRef::data_type() const {
  return service_ ? service_->GetDataType(id_) : nullptr;
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
  return service_ ? service_->GetTarget(id_, reference_type_id, true) : nullptr;
}

NodeRef NodeRef::inverse_target(const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetTarget(id_, reference_type_id, false)
                  : nullptr;
}

bool NodeRef::has_target(const scada::NodeId& reference_type_id,
                         bool forward,
                         const scada::NodeId& node_id) const {
  return !!reference(reference_type_id, forward, node_id).target;
}

std::vector<NodeRef> NodeRef::targets(
    const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetTargets(id_, reference_type_id, true)
                  : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::inverse_targets(
    const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetTargets(id_, reference_type_id, false)
                  : std::vector<NodeRef>{};
}

NodeRef::Reference NodeRef::reference(
    const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetReference(id_, reference_type_id, true, {})
                  : NodeRef::Reference{};
}

NodeRef::Reference NodeRef::inverse_reference(
    const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetReference(id_, reference_type_id, false, {})
                  : NodeRef::Reference{};
}

NodeRef::Reference NodeRef::reference(const scada::NodeId& reference_type_id,
                                      bool forward,
                                      const scada::NodeId& node_id) const {
  return service_
             ? service_->GetReference(id_, reference_type_id, forward, node_id)
             : NodeRef::Reference{};
}

std::vector<NodeRef::Reference> NodeRef::references(
    const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetReferences(id_, reference_type_id, true)
                  : std::vector<NodeRef::Reference>{};
}

std::vector<NodeRef::Reference> NodeRef::inverse_references(
    const scada::NodeId& reference_type_id) const {
  return service_ ? service_->GetReferences(id_, reference_type_id, false)
                  : std::vector<NodeRef::Reference>{};
}

NodeRef NodeRef::operator[](
    const scada::NodeId& aggregate_declaration_id) const {
  return service_ ? service_->GetAggregate(id_, aggregate_declaration_id)
                  : nullptr;
}

NodeRef NodeRef::operator[](const scada::QualifiedName& child_name) const {
  return service_ ? service_->GetChild(id_, child_name) : NodeRef{};
}

boost::signals2::scoped_connection NodeRef::SubscribeModelChanged(
    const ModelChangedCallback& callback) const {
  return service_ ? service_->SubscribeModelChanged(id_, callback)
                  : boost::signals2::scoped_connection{};
}

boost::signals2::scoped_connection NodeRef::SubscribeNodeSemanticChanged(
    const NodeSemanticChangedCallback& callback) const {
  return service_ ? service_->SubscribeNodeSemanticChanged(id_, callback)
                  : boost::signals2::scoped_connection{};
}

boost::signals2::scoped_connection NodeRef::SubscribeNodeFetched(
    const NodeFetchedCallback& callback) const {
  return service_ ? service_->SubscribeNodeFetched(id_, callback)
                  : boost::signals2::scoped_connection{};
}

boost::signals2::scoped_connection NodeRef::SubscribeNodeStateChanged(
    const NodeStateChangedCallback& callback) const {
  return service_ ? service_->SubscribeNodeStateChanged(id_, callback)
                  : boost::signals2::scoped_connection{};
}

scada::node NodeRef::scada_node() const {
  return service_ ? service_->GetScadaNode(id_) : scada::node{};
}
