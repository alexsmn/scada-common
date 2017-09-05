#include "common/node_ref.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_ref_data.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"

namespace {

std::vector<NodeRef> GetTargets(const NodeRef::References& references, const scada::NodeId& reference_type_id) {
  std::vector<NodeRef> result;
  result.reserve(references.size());
  for (auto& ref : references) {
    assert(ref.reference_type.fetched());
    assert(ref.target.fetched());
    if (IsSubtypeOf(ref.reference_type, reference_type_id))
      result.emplace_back(ref.target);
  }
  return result;
}

} // namespace

NodeRef::NodeRef(std::shared_ptr<NodeRefData> data)
    : data_{std::move(data)} {
}

scada::NodeId NodeRef::id() const {
  return data_ ? data_->id : scada::NodeId{};
}

base::Optional<scada::NodeClass> NodeRef::node_class() const {
  if (data_ && data_->fetched)
    return data_->node_class;
  else
   return {};
}

bool NodeRef::fetched() const {
  return !data_ || data_->fetched;
}

NodeRef NodeRef::data_type() const {
  return data_ ? data_->data_type : nullptr;
}

std::string NodeRef::browse_name() const {
  if (!data_)
    return {};
  return data_->fetched ? data_->browse_name : data_->id.ToString();
}

base::string16 NodeRef::display_name() const {
  if (!data_)
    return {};
  if (!data_->fetched)
    return base::SysNativeMBToWide(data_->id.ToString());
  if (data_->display_name.empty())
    return base::SysNativeMBToWide(data_->browse_name);
  return data_->display_name;
}

NodeRef NodeRef::type_definition() const {
  return data_ && data_->fetched ? data_->type_definition : nullptr;
}

NodeRef NodeRef::supertype() const {
  return data_ && data_->fetched ? data_->supertype : nullptr;
}

std::vector<NodeRef> NodeRef::aggregates() const {
  return data_ && data_->fetched ? GetTargets(data_->aggregates, OpcUaId_Aggregates) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::components() const {
  return data_ && data_->fetched ? GetTargets(data_->aggregates, OpcUaId_HasComponent) : std::vector<NodeRef>{};
}

std::vector<NodeRef> NodeRef::properties() const {
  return data_ && data_->fetched ? GetTargets(data_->aggregates, OpcUaId_HasProperty) : std::vector<NodeRef>{};
}

std::vector<NodeRef::Reference> NodeRef::references() const {
  return data_ && data_->fetched ? data_->references : std::vector<Reference>{};
}

NodeRef NodeRef::operator[](base::StringPiece aggregate_name) const {
  if (!data_ || !data_->fetched)
    return nullptr;
  const auto& aggregates = data_->aggregates;
  auto i = std::find_if(aggregates.begin(), aggregates.end(),
      [aggregate_name](const NodeRef::Reference& reference) {
        assert(reference.reference_type.fetched());
        assert(reference.target.fetched());
        return reference.target.browse_name() == aggregate_name;
      });
  return i == aggregates.end() ? nullptr : i->target;
}

NodeRef NodeRef::operator[](const scada::NodeId& aggregate_declaration_id) const {
  if (!data_ || !data_->fetched)
    return nullptr;
  auto aggregate_declaration = GetAggregateDeclaration(aggregate_declaration_id);
  if (!aggregate_declaration)
    return nullptr;
  return (*this)[aggregate_declaration.browse_name()];
}

scada::DataValue NodeRef::data_value() const {
  if (!data_ || !data_->fetched)
    return {};
  return data_->data_value;
}

NodeRef NodeRef::target(const scada::NodeId& reference_type_id) const {
  if (!data_ || !data_->fetched)
    return nullptr;
  for (auto& p : data_->references) {
    assert(p.reference_type.fetched());
    if (IsSubtypeOf(p.reference_type, reference_type_id))
      return p.target;
  }
  return nullptr;
}

std::vector<NodeRef> NodeRef::targets(const scada::NodeId& reference_type_id) const {
  if (!data_ || !data_->fetched)
    return std::vector<NodeRef>{};
  return GetTargets(data_->references, reference_type_id);
}

NodeRef NodeRef::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  if (!data_ || !data_->fetched)
    return nullptr;
  auto type_definition = scada::IsTypeDefinition(data_->node_class) ? *this : data_->type_definition;
  for (; type_definition; type_definition = type_definition.supertype()) {
    assert(type_definition.fetched());
    const auto& declarations = type_definition.data_->aggregates;
    auto i = std::find_if(declarations.begin(), declarations.end(),
        [&aggregate_declaration_id](const NodeRef::Reference& reference) {
          return reference.target.id() == aggregate_declaration_id;
        });
    if (i != declarations.end())
      return i->target;
  }
  return nullptr;
}
