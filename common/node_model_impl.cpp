#include "node_model_impl.h"

#include "base/logger.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_ref_service_impl.h"
#include "common/node_ref_util.h"
#include "core/attribute_service.h"

namespace {

constexpr scada::AttributeId kReadAttributeIds[] = {
    scada::AttributeId::NodeClass,
    scada::AttributeId::BrowseName,
    scada::AttributeId::DisplayName,
    scada::AttributeId::DataType,
    scada::AttributeId::Value,
};

} // namespace

NodeModelImpl::NodeModelImpl(NodeRefServiceImpl& service, scada::NodeId id, std::shared_ptr<const Logger> logger)
    : service_{service},
      id_{std::move(id)},
      logger_{std::move(logger)} {
}

NodeRef NodeModelImpl::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  if (!fetched_ || !status_)
    return nullptr;

  assert(node_class_.has_value());
  auto* type_definition = scada::IsTypeDefinition(*node_class_) ? this : type_definition_.get();
  for (; type_definition; type_definition = type_definition->supertype_.get()) {
    assert(type_definition->fetched_);
    const auto& declarations = type_definition->aggregates_;
    auto i = std::find_if(declarations.begin(), declarations.end(),
        [&aggregate_declaration_id](const NodeModelImplReference& reference) {
          return reference.target->id_ == aggregate_declaration_id;
        });
    if (i != declarations.end())
      return i->target;
  }

  return nullptr;
}

NodeRef NodeModelImpl::GetAggregate(const scada::QualifiedName& aggregate_name) const {
  if (!fetched_)
    return nullptr;

  auto i = std::find_if(aggregates_.begin(), aggregates_.end(),
      [aggregate_name](const NodeModelImplReference& reference) {
        assert(reference.reference_type->fetched_);
        assert(reference.target->fetched_);
        return reference.target->browse_name_ == aggregate_name;
      });
  return i == aggregates_.end() ? nullptr : i->target;
}

std::vector<NodeRef> NodeModelImpl::GetAggregates(const scada::NodeId& reference_type_id) const {
  std::vector<NodeRef> result;
  if (!fetched_)
    return result;
  result.reserve(aggregates_.size());
  for (auto& ref : aggregates_) {
    assert(ref.reference_type->fetched_);
    assert(ref.target->fetched_);
    if (IsSubtypeOf(ref.reference_type, reference_type_id))
      result.emplace_back(ref.target);
  }
  return result;
}

NodeRef NodeModelImpl::GetTarget(const scada::NodeId& reference_type_id) const {
  if (!fetched_)
    return nullptr;
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    if (IsSubtypeOf(ref.reference_type, reference_type_id))
      return ref.target;
  }
  return nullptr;
}

std::vector<NodeRef> NodeModelImpl::GetTargets(const scada::NodeId& reference_type_id) const {
  std::vector<NodeRef> result;
  if (!fetched_)
    return result;
  result.reserve(references_.size());
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    assert(ref.target->fetched_);
    if (IsSubtypeOf(ref.reference_type, reference_type_id))
      result.emplace_back(ref.target);
  }
  return result;
}

std::vector<NodeRef::Reference> NodeModelImpl::GetReferences() const {
  std::vector<NodeRef::Reference> result;
  if (!fetched_)
    return result;
  result.reserve(references_.size());
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    assert(ref.target->fetched_);
    result.emplace_back(NodeRef::Reference{ref.reference_type, ref.target, ref.forward});
  }
  return result;
}

void NodeModelImpl::Fetch(const NodeRef::FetchCallback& callback) const {
  if (fetched_) {
    if (callback)
      callback(shared_from_this());
    return;
  }

  if (callback)
    fetch_callbacks_.emplace_back(callback);

  if (pending_request_count_ != 0)
    return;

  logger_->WriteF(LogSeverity::Normal, "Request node %s", id_.ToString().c_str());

  std::vector<scada::ReadValueId> read_ids(std::size(kReadAttributeIds));
  for (size_t i = 0; i < read_ids.size(); ++i)
    read_ids[i] = scada::ReadValueId{id_, kReadAttributeIds[i]};

  // TODO: Weak ptr.
  ++pending_request_count_;
  std::weak_ptr<NodeModelImpl> weak_ptr = std::const_pointer_cast<NodeModelImpl>(shared_from_this());
  service_.attribute_service_.Read(read_ids, [weak_ptr](scada::Status&& status, std::vector<scada::DataValue>&& data_values) {
    if (auto ptr = weak_ptr.lock())
      ptr->OnReadComplete(std::move(status), std::move(data_values));
  });
}

scada::Variant NodeModelImpl::GetAttribute(scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case scada::AttributeId::NodeId:
      return id_;

    case scada::AttributeId::NodeClass:
      return node_class_.has_value() ? scada::Variant{static_cast<int>(*node_class_)} : scada::Variant{};

    case scada::AttributeId::BrowseName:
      if (!fetched_ || !status_)
        return scada::QualifiedName{id_.ToString()};
      assert(!browse_name_.empty());
      return browse_name_;

    case scada::AttributeId::DisplayName:
      if (!fetched_)
        return scada::LocalizedText{id_.ToString()};
      if (!status_)
        return scada::LocalizedText{status_.ToString()};
      else if (!display_name_.empty())
        return display_name_;
      else
        return scada::LocalizedText{browse_name_.name()};

    default:
      assert(false);
      return {};
  }
}

scada::Variant NodeModelImpl::GetValue() const {
  return value_;
}

NodeRef NodeModelImpl::GetTypeDefinition() const {
  return type_definition_;
}

NodeRef NodeModelImpl::GetSupertype() const {
  return supertype_;
}

NodeRef NodeModelImpl::GetDataType() const {
  return data_type_;
}

scada::Status NodeModelImpl::GetStatus() const {
  return status_;
}

void NodeModelImpl::Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback) const {
  assert(description.node_id == id_);
  service_.Browse(description, callback);
}

void NodeModelImpl::AddObserver(NodeRefObserver& observer) const {
  service_.AddNodeObserver(id_, observer);
}

void NodeModelImpl::RemoveObserver(NodeRefObserver& observer) const {
  service_.RemoveNodeObserver(id_, observer);
}

void NodeModelImpl::OnReadComplete(scada::Status&& status, std::vector<scada::DataValue>&& data_values) {
  // Request could be canceled.
  if (!status_)
    return;

  assert(pending_request_count_ > 0);
  --pending_request_count_;

  if (!status) {
    logger_->WriteF(LogSeverity::Warning, "Read failed for node %s", id_.ToString().c_str());
    SetError(status);
    return;
  }

  assert(data_values.size() == std::size(kReadAttributeIds));

  logger_->WriteF(LogSeverity::Normal, "Read complete for node %s", id_.ToString().c_str());

  for (size_t i = 0; i < data_values.size(); ++i) {
    const auto attribute_id = kReadAttributeIds[i];
    auto& data_value = data_values[i];
    // assert(scada::IsGood(value.status_code) || attribute_id != OpcUa_Attributes_BrowseName);
    if (scada::IsGood(data_value.status_code))
      SetAttribute(attribute_id, std::move(data_value.value));
  }

  if (!node_class_.has_value() || browse_name_.empty()) {
    logger_->WriteF(LogSeverity::Warning, "Node %s attributes weren't read", id_.ToString().c_str());
    static_assert(kReadAttributeIds[0] == scada::AttributeId::NodeClass);
    static_assert(kReadAttributeIds[1] == scada::AttributeId::BrowseName);
    auto& node_class_status = data_values[0].status_code;
    auto& browse_name_status = data_values[0].status_code;
    assert(!scada::IsGood(node_class_status) || !scada::IsGood(browse_name_status));
    SetError(scada::IsGood(node_class_status) ? node_class_status : browse_name_status);
    return;
  }

  // TODO: Data integrity check.

  std::vector<scada::BrowseDescription> nodes;
  nodes.reserve(3);
  assert(node_class_.has_value());
  if (scada::IsTypeDefinition(*node_class_))
    nodes.push_back({id_, scada::BrowseDirection::Inverse, scada::id::HasSubtype, true});
  nodes.push_back({id_, scada::BrowseDirection::Forward, scada::id::Aggregates, true});
  nodes.push_back({id_, scada::BrowseDirection::Forward, scada::id::NonHierarchicalReferences, true});

  ++pending_request_count_;
  std::weak_ptr<NodeModelImpl> weak_ptr = shared_from_this();
  service_.view_service_.Browse(std::move(nodes),
      [weak_ptr](scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        if (auto ptr = weak_ptr.lock())
          ptr->OnBrowseComplete(std::move(status), std::move(results));
      });
}

void NodeModelImpl::OnBrowseComplete(scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
  // Request could be canceled.
  if (!status_)
    return;

  assert(pending_request_count_ > 0);
  --pending_request_count_;

  if (!status) {
    logger_->WriteF(LogSeverity::Normal, "Browse failed for node %s", id_.ToString().c_str());
    SetError(status);
    return;
  }

  logger_->WriteF(LogSeverity::Normal, "Browse complete for node %s", id_.ToString().c_str());

  for (auto& result : results) {
    for (auto& reference : result.references) {
      pending_references_.push_back({
          service_.GetNodeImpl(reference.reference_type_id, id_),
          service_.GetNodeImpl(reference.node_id, id_),
          reference.forward,
      });
    }
  }

  // TODO: Integrity check.

  service_.CompletePartialNode(shared_from_this());
}

void NodeModelImpl::SetAttribute(scada::AttributeId attribute_id, scada::Variant&& value) {
  switch (attribute_id) {
    case scada::AttributeId::NodeClass:
      node_class_ = static_cast<scada::NodeClass>(value.as_int32());
      break;
    case scada::AttributeId::BrowseName:
      browse_name_ = std::move(value.get<scada::QualifiedName>());
      assert(!browse_name_.empty());
      break;
    case scada::AttributeId::DisplayName:
      display_name_ = std::move(value.get<scada::LocalizedText>());
      assert(!display_name_.empty());
      break;
    case scada::AttributeId::DataType:
      data_type_ = service_.GetNodeImpl(value.as_node_id(), id_);
      break;
    case scada::AttributeId::Value:
      value_ = std::move(value);
      break;
  }
}

void NodeModelImpl::AddReference(const NodeModelImplReference& reference) {
//  assert(reference.reference_type.fetched());
//  assert(reference.target.fetched());

  if (reference.forward) {
    if (IsSubtypeOf(reference.reference_type, scada::id::HasTypeDefinition))
      type_definition_ = reference.target;
    else if (IsSubtypeOf(reference.reference_type, scada::id::Aggregates))
      aggregates_.push_back(reference);
    else if (IsSubtypeOf(reference.reference_type, scada::id::NonHierarchicalReferences))
      references_.push_back(reference);

  } else {
    if (IsSubtypeOf(reference.reference_type, scada::id::HasSubtype))
      supertype_ = reference.target;
  }
}

bool NodeModelImpl::IsNodeFetched(std::vector<scada::NodeId>& fetched_node_ids) const {
  if (fetched_)
    return true;

  if (passing_)
    return true;

  if (std::find(fetched_node_ids.begin(), fetched_node_ids.end(), id_) != fetched_node_ids.end())
    return true;

  passing_ = true;

  bool fetched = IsNodeFetchedHelper(fetched_node_ids);

  assert(passing_);
  passing_ = false;

  if (fetched) {
    assert(std::find(fetched_node_ids.begin(), fetched_node_ids.end(), id_) == fetched_node_ids.end());
    fetched_node_ids.emplace_back(id_);
  }

  return fetched;
}

bool NodeModelImpl::IsNodeFetchedHelper(std::vector<scada::NodeId>& fetched_node_ids) const {
  if (pending_request_count_ != 0)
    return false;

  if (data_type_ && !data_type_->IsNodeFetched(fetched_node_ids))
    return false;

  for (auto& reference : pending_references_) {
    if (!reference.reference_type->IsNodeFetched(fetched_node_ids))
      return false;
    if (!reference.target->IsNodeFetched(fetched_node_ids))
      return false;
  }
  return true;
}

void NodeModelImpl::SetError(const scada::Status& status) {
  assert(status_.good());

  logger_->WriteF(LogSeverity::Warning, "Node %s error %s", id_.ToString().c_str(), status.ToString().c_str());

  status_ = status;
  pending_request_count_ = 0;
  service_.CompletePartialNode(shared_from_this());
}
