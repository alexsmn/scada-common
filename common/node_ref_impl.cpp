#include "node_ref_impl.h"

#include "base/logger.h"
#include "base/strings/sys_string_conversions.h"
#include "common/node_ref_impl.h"
#include "common/node_ref_service_impl.h"
#include "common/node_ref_util.h"
#include "core/attribute_service.h"

namespace {

constexpr scada::AttributeId kReadAttributeIds[] = {
    OpcUa_Attributes_NodeClass,
    OpcUa_Attributes_BrowseName,
    OpcUa_Attributes_DisplayName,
    OpcUa_Attributes_DataType,
    OpcUa_Attributes_Value,
};

} // namespace

NodeRefImpl::NodeRefImpl(NodeRefServiceImpl& service, scada::NodeId id, std::shared_ptr<const Logger> logger)
    : service_{service},
      id_{std::move(id)},
      logger_{std::move(logger)} {
}

NodeRef NodeRefImpl::GetAggregateDeclaration(const scada::NodeId& aggregate_declaration_id) const {
  if (!fetched_)
    return nullptr;

  assert(node_class_.has_value());
  auto* type_definition = scada::IsTypeDefinition(*node_class_) ? this : type_definition_.get();
  for (; type_definition; type_definition = type_definition->supertype_.get()) {
    assert(type_definition->fetched_);
    const auto& declarations = type_definition->aggregates_;
    auto i = std::find_if(declarations.begin(), declarations.end(),
        [&aggregate_declaration_id](const NodeRefImplReference& reference) {
          return reference.target->id_ == aggregate_declaration_id;
        });
    if (i != declarations.end())
      return i->target;
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
  return i == aggregates_.end() ? nullptr : i->target;
}

std::vector<NodeRef> NodeRefImpl::GetAggregates(const scada::NodeId& reference_type_id) const {
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

NodeRef NodeRefImpl::GetTarget(const scada::NodeId& reference_type_id) const {
  if (!fetched_)
    return nullptr;
  for (auto& ref : references_) {
    assert(ref.reference_type->fetched_);
    if (IsSubtypeOf(ref.reference_type, reference_type_id))
      return ref.target;
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
    if (IsSubtypeOf(ref.reference_type, reference_type_id))
      result.emplace_back(ref.target);
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
    result.emplace_back(NodeRef::Reference{ref.reference_type, ref.target, ref.forward});
  }
  return result;
}

void NodeRefImpl::Fetch(const NodeRef::FetchCallback& callback) {
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
  std::weak_ptr<NodeRefImpl> weak_ptr = shared_from_this();
  service_.attribute_service_.Read(read_ids, [weak_ptr](const scada::Status& status, std::vector<scada::DataValue> values) {
    if (auto ptr = weak_ptr.lock())
      ptr->OnReadComplete(status, std::move(values));
  });
}

scada::Variant NodeRefImpl::GetAttribute(scada::AttributeId attribute_id) const {
  switch (attribute_id) {
    case OpcUa_Attributes_NodeId:
      return id_;

    case OpcUa_Attributes_NodeClass:
      return node_class_.has_value() ? scada::Variant{static_cast<int>(*node_class_)} : scada::Variant{};

    case OpcUa_Attributes_BrowseName:
      if (!fetched_ || !status_)
        return scada::QualifiedName{id_.ToString()};
      assert(!browse_name_.empty());
      return browse_name_;

    case OpcUa_Attributes_DisplayName:
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

scada::DataValue NodeRefImpl::GetValue() const {
  return data_value_;
}

NodeRef NodeRefImpl::GetTypeDefinition() const {
  return type_definition_;
}

NodeRef NodeRefImpl::GetSupertype() const {
  return supertype_;
}

NodeRef NodeRefImpl::GetDataType() const {
  return data_type_;
}

scada::Status NodeRefImpl::GetStatus() const {
  return status_;
}

void NodeRefImpl::Browse(const scada::BrowseDescription& description, const NodeRef::BrowseCallback& callback) const {
  assert(description.node_id == id_);
  service_.Browse(description, callback);
}

void NodeRefImpl::AddObserver(NodeRefObserver& observer) {
  service_.AddNodeObserver(id_, observer);
}

void NodeRefImpl::RemoveObserver(NodeRefObserver& observer) {
  service_.RemoveNodeObserver(id_, observer);
}

void NodeRefImpl::OnReadComplete(const scada::Status& status, std::vector<scada::DataValue> values) {
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

  assert(values.size() == std::size(kReadAttributeIds));

  logger_->WriteF(LogSeverity::Normal, "Read complete for node %s", id_.ToString().c_str());

  for (size_t i = 0; i < values.size(); ++i) {
    const auto attribute_id = kReadAttributeIds[i];
    auto& value = values[i];
    // assert(scada::IsGood(value.status_code) || attribute_id != OpcUa_Attributes_BrowseName);
    if (scada::IsGood(value.status_code))
      SetAttribute(attribute_id, std::move(value));
  }

  if (!node_class_.has_value() || browse_name_.empty()) {
    logger_->WriteF(LogSeverity::Warning, "Node %s attributes weren't read", id_.ToString().c_str());
    static_assert(kReadAttributeIds[0] == OpcUa_Attributes_NodeClass);
    static_assert(kReadAttributeIds[1] == OpcUa_Attributes_BrowseName);
    auto& node_class_status = values[0].status_code;
    auto& browse_name_status = values[0].status_code;
    assert(!scada::IsGood(node_class_status) || !scada::IsGood(browse_name_status));
    SetError(scada::IsGood(node_class_status) ? node_class_status : browse_name_status);
    return;
  }

  // TODO: Data integrity check.

  std::vector<scada::BrowseDescription> nodes;
  nodes.reserve(3);
  assert(node_class_.has_value());
  if (scada::IsTypeDefinition(*node_class_))
    nodes.push_back({id_, scada::BrowseDirection::Inverse, OpcUaId_HasSubtype, true});
  nodes.push_back({id_, scada::BrowseDirection::Forward, OpcUaId_Aggregates, true});
  nodes.push_back({id_, scada::BrowseDirection::Forward, OpcUaId_NonHierarchicalReferences, true});

  ++pending_request_count_;
  std::weak_ptr<NodeRefImpl> weak_ptr = shared_from_this();
  service_.view_service_.Browse(std::move(nodes),
      [weak_ptr](const scada::Status& status, std::vector<scada::BrowseResult> results) {
        if (auto ptr = weak_ptr.lock())
          ptr->OnBrowseComplete(status, std::move(results));
      });
}

void NodeRefImpl::OnBrowseComplete(const scada::Status& status, std::vector<scada::BrowseResult> results) {
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

void NodeRefImpl::SetAttribute(scada::AttributeId attribute_id, scada::DataValue data_value) {
  switch (attribute_id) {
    case OpcUa_Attributes_NodeClass:
      node_class_ = static_cast<scada::NodeClass>(data_value.value.as_int32());
      break;
    case OpcUa_Attributes_BrowseName:
      browse_name_ = std::move(data_value.value.get<scada::QualifiedName>());
      assert(!browse_name_.empty());
      break;
    case OpcUa_Attributes_DisplayName:
      display_name_ = std::move(data_value.value.get<scada::LocalizedText>());
      assert(!display_name_.empty());
      break;
    case OpcUa_Attributes_DataType:
      data_type_ = service_.GetNodeImpl(data_value.value.as_node_id(), id_);
      break;
    case OpcUa_Attributes_Value:
      data_value_ = std::move(data_value);
      break;
  }
}

void NodeRefImpl::AddReference(const NodeRefImplReference& reference) {
//  assert(reference.reference_type.fetched());
//  assert(reference.target.fetched());

  if (reference.forward) {
    if (IsSubtypeOf(reference.reference_type, OpcUaId_HasTypeDefinition))
      type_definition_ = reference.target;
    else if (IsSubtypeOf(reference.reference_type, OpcUaId_Aggregates))
      aggregates_.push_back(reference);
    else if (IsSubtypeOf(reference.reference_type, OpcUaId_NonHierarchicalReferences))
      references_.push_back(reference);

  } else {
    if (IsSubtypeOf(reference.reference_type, OpcUaId_HasSubtype))
      supertype_ = reference.target;
  }
}

bool NodeRefImpl::IsNodeFetched(std::vector<scada::NodeId>& fetched_node_ids) {
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

bool NodeRefImpl::IsNodeFetchedHelper(std::vector<scada::NodeId>& fetched_node_ids) {
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

void NodeRefImpl::SetError(const scada::Status& status) {
  assert(status_.good());

  logger_->WriteF(LogSeverity::Warning, "Node %s error %s", id_.ToString().c_str(), status.ToString().c_str());

  status_ = status;
  pending_request_count_ = 0;
  service_.CompletePartialNode(shared_from_this());
}
