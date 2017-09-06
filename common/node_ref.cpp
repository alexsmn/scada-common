#include "common/node_ref.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_ref_impl.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"

namespace {

std::vector<NodeRef> GetTargets(const std::vector<NodeRefImplReference>& references, const scada::NodeId& reference_type_id) {
  std::vector<NodeRef> result;
  result.reserve(references.size());
  for (auto& ref : references) {
    assert(ref.reference_type->fetched);
    assert(ref.target->fetched);
    if (IsSubtypeOf(NodeRef{ref.reference_type}, reference_type_id))
      result.emplace_back(NodeRef{ref.target});
  }
  return result;
}

std::vector<NodeRef::Reference> GetReferences(const std::vector<NodeRefImplReference>& references) {
  std::vector<NodeRef::Reference> result;
  result.reserve(references.size());
  for (auto& ref : references) {
    assert(ref.reference_type->fetched);
    assert(ref.target->fetched);
    result.emplace_back(NodeRef::Reference{NodeRef{ref.reference_type}, NodeRef{ref.target}, ref.forward});
  }
  return result;
}

} // namespace

NodeRef::NodeRef(std::shared_ptr<NodeRefImpl> impl)
    : impl_{std::move(impl)} {
}

scada::NodeId NodeRef::id() const {
  return impl_ ? impl_->id() : scada::NodeId{};
}

base::Optional<scada::NodeClass> NodeRef::node_class() const {
  if (impl_ && impl_->fetched)
    return impl_->node_class;
  else
   return {};
}

bool NodeRef::fetched() const {
  return !impl_ || impl_->fetched;
}

NodeRef NodeRef::data_type() const {
  return impl_ ? NodeRef{impl_->data_type} : nullptr;
}

std::string NodeRef::browse_name() const {
  if (!impl_)
    return {};
  return impl_->fetched ? impl_->browse_name : impl_->id().ToString();
}

base::string16 NodeRef::display_name() const {
  if (!impl_)
    return {};
  if (!impl_->fetched)
    return base::SysNativeMBToWide(impl_->id().ToString());
  if (impl_->display_name.empty())
    return base::SysNativeMBToWide(impl_->browse_name);
  return impl_->display_name;
}

NodeRef NodeRef::type_definition() const {
  return impl_ && impl_->fetched ? NodeRef{impl_->type_definition} : nullptr;
}

NodeRef NodeRef::supertype() const {
  return impl_ && impl_->fetched ? NodeRef{impl_->supertype} : nullptr;
}

std::vector<NodeRef> NodeRef::aggregates() const {
  return impl_ && impl_->fetched ? GetTargets(impl_->aggregates, OpcUaId_Aggregates) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::components() const {
  return impl_ && impl_->fetched ? GetTargets(impl_->aggregates, OpcUaId_HasComponent) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::properties() const {
  return impl_ && impl_->fetched ? GetTargets(impl_->aggregates, OpcUaId_HasProperty) : std::vector<NodeRef>{};
}

std::vector<NodeRef::Reference> NodeRef::references() const {
  return impl_ && impl_->fetched ? GetReferences(impl_->references) : std::vector<Reference>{};
}

NodeRef NodeRef::operator[](base::StringPiece aggregate_name) const {
  if (!impl_ || !impl_->fetched)
    return nullptr;
  const auto& aggregates = impl_->aggregates;
  auto i = std::find_if(aggregates.begin(), aggregates.end(),
      [aggregate_name](const NodeRefImplReference& reference) {
        assert(reference.reference_type->fetched);
        assert(reference.target->fetched);
        return reference.target->browse_name == aggregate_name;
      });
  return i == aggregates.end() ? nullptr : NodeRef{i->target};
}

NodeRef NodeRef::operator[](const scada::NodeId& aggregate_declaration_id) const {
  if (!impl_ || !impl_->fetched)
    return nullptr;
  auto aggregate_declaration = GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;
  return (*this)[aggregate_declaration.browse_name()];
}

scada::DataValue NodeRef::data_value() const {
  if (!impl_ || !impl_->fetched)
    return {};
  return impl_->data_value;
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  if (!impl_ || !impl_->fetched)
    return nullptr;
  for (auto& ref : impl_->references) {
    assert(ref.reference_type->fetched);
    if (IsSubtypeOf(NodeRef{ref.reference_type}, reference_type_id))
      return NodeRef{ref.target};
  }
  return nullptr;
}

std::vector<NodeRef> NodeRef::targets(const scada::NodeId& reference_type_id) const {
  if (!impl_ || !impl_->fetched)
    return std::vector<NodeRef>{};
  return GetTargets(impl_->references, reference_type_id);
}

NodeRef NodeRef::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  if (!impl_ || !impl_->fetched)
    return nullptr;
  auto type_definition = scada::IsTypeDefinition(impl_->node_class) ? *this : NodeRef{impl_->type_definition};
  for (; type_definition; type_definition = type_definition.supertype()) {
    assert(type_definition.fetched());
    const auto& declarations = type_definition.impl_->aggregates;
    auto i = std::find_if(declarations.begin(), declarations.end(),
        [&aggregate_declaration_id](const NodeRefImplReference& reference) {
          return reference.target->id() == aggregate_declaration_id;
        });
    if (i != declarations.end())
      return NodeRef{i->target};
  }
  return nullptr;
}
