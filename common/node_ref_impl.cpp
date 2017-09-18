#include "node_ref_impl.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_ref_impl.h"
#include "common/node_ref_service_impl.h"
#include "common/node_ref_util.h"

NodeRef NodeRefImpl::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  if (!fetched_)
    return nullptr;

  auto* type_definition = scada::IsTypeDefinition(node_class_) ? this : type_definition_.get();
  for (; type_definition; type_definition = type_definition->supertype_.get()) {
    assert(type_definition->fetched_);
    const auto& declarations = type_definition->aggregates_;
    auto i = std::find_if(declarations.begin(), declarations.end(),
        [&aggregate_declaration_id](const NodeRefImplReference& reference) {
          return reference.target->id_ == aggregate_declaration_id;
        });
    if (i != declarations.end())
      return NodeRef{i->target};
  }

  return nullptr;
}

NodeRef NodeRefImpl::GetAggregate(const scada::QualifiedName& aggregate_name) const {
  if (!fetched_)
    return nullptr;

  auto i = std::find_if(aggregates_.begin(), aggregates_.end(),
      [aggregate_name](const NodeRefImplReference& reference) {
        assert(reference.reference_type->fetched_);
        assert(reference.target->fetched_);
        return reference.target->browse_name_ == aggregate_name;
      });
  return i == aggregates_.end() ? nullptr : NodeRef{i->target};
}

std::vector<NodeRef> NodeRefImpl::GetAggregates(const scada::NodeId& reference_type_id) const {
  std::vector<NodeRef> result;
  if (!fetched_)
    return result;
  result.reserve(aggregates_.size());
  for (auto& ref : aggregates_) {
    assert(ref.reference_type->fetched_);
    assert(ref.target->fetched_);
    if (IsSubtypeOf(NodeRef{ref.reference_type}, reference_type_id))
      result.emplace_back(NodeRef{ref.target});
  }
  return result;
}

NodeRef NodeRefImpl::GetTarget(const scada::NodeId& reference_type_id) const {
  if (!fetched_)
    return nullptr;
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    if (IsSubtypeOf(NodeRef{ref.reference_type}, reference_type_id))
      return NodeRef{ref.target};
  }
  return nullptr;
}

std::vector<NodeRef> NodeRefImpl::GetTargets(const scada::NodeId& reference_type_id) const {
  std::vector<NodeRef> result;
  if (!fetched_)
    return result;
  result.reserve(references_.size());
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    assert(ref.target->fetched_);
    if (IsSubtypeOf(NodeRef{ref.reference_type}, reference_type_id))
      result.emplace_back(NodeRef{ref.target});
  }
  return result;
}

std::vector<NodeRef::Reference> NodeRefImpl::GetReferences() const {
  std::vector<NodeRef::Reference> result;
  if (!fetched_)
    return result;
  result.reserve(references_.size());
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    assert(ref.target->fetched_);
    result.emplace_back(NodeRef::Reference{NodeRef{ref.reference_type}, NodeRef{ref.target}, ref.forward});
  }
  return result;
}

void NodeRefImpl::Fetch(const NodeRef::FetchCallback& callback) {
  NodeRef node{shared_from_this()};
  if (fetched_)
    callback(std::move(node));
  else
    service_.RequestNode(id_, [callback, node] { callback(std::move(node)); });
}

scada::DataValue NodeRefImpl::GetAttribute(scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case OpcUa_Attributes_NodeId:
      return {id_, {}, {}, {}};

    case OpcUa_Attributes_BrowseName: {
      scada::QualifiedName browse_name = fetched_ ? browse_name_ : scada::QualifiedName{id_.ToString(), 0};
      return {std::move(browse_name), {}, {}, {}};
    }

    case OpcUa_Attributes_DisplayName: {
      scada::LocalizedText display_name;
      if (!fetched_)
        display_name = id_.ToString();
      else if (display_name_.empty())
        display_name = browse_name_.name();
      else
        display_name = display_name_;
      return {std::move(display_name), {}, {}, {}};
    }

    case OpcUa_Attributes_Value:
      return data_value_;

    default:
      assert(false);
      return {};
  }
}

NodeRef NodeRefImpl::GetTypeDefinition() const {
  return NodeRef{type_definition_};
}

NodeRef NodeRefImpl::GetSupertype() const {
  return NodeRef{supertype_};
}

NodeRef NodeRefImpl::GetDataType() const {
  return NodeRef{data_type_};
}

scada::Status NodeRefImpl::GetStatus() const {
  return status_;
}

void NodeRefImpl::Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback) {
  assert(description.node_id == id_);
  service_.Browse(description, callback);
}

void NodeRefImpl::AddObserver(NodeRefObserver& observer) {
  service_.AddNodeObserver(id_, observer);
}

void NodeRefImpl::RemoveObserver(NodeRefObserver& observer) {
  service_.RemoveNodeObserver(id_, observer);
}
