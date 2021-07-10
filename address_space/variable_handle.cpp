#include "address_space/variable_handle.h"

#include "core/data_value.h"

namespace scada {

std::shared_ptr<VariableMonitoredItem> CreateMonitoredVariable(
    std::shared_ptr<VariableHandle> variable) {
  if (!variable)
    return nullptr;
  return std::make_shared<VariableMonitoredItem>(std::move(variable));
}

// VariableMonitoredItem

VariableMonitoredItem::VariableMonitoredItem(
    std::shared_ptr<VariableHandle> variable)
    : variable_(std::move(variable)) {
  variable_->monitored_items_.insert(this);
}

VariableMonitoredItem::~VariableMonitoredItem() {
  variable_->monitored_items_.erase(this);
}

void VariableMonitoredItem::Subscribe() {
  assert(!subscribed_);
  subscribed_ = true;
  if (!variable_->last_value().server_timestamp.is_null())
    ForwardData(variable_->last_value());
}

// VariableHandle

VariableHandle::~VariableHandle() {
  assert(monitored_items_.empty());
}

void VariableHandle::ForwardData(const DataValue& value) {
  if (value == last_value_)
    return;

  // Update current value only with latest time.
  if (IsUpdate(last_value_, value)) {
    bool is_change = last_value_.value != value.value ||
                     last_value_.qualifier != value.qualifier;
    if (is_change)
      last_change_time_ = value.source_timestamp;

    last_value_ = value;
  }

  if (monitored_items_.empty())
    return;

  // Prevent from deletion.
  auto ref = shared_from_this();

  for (auto i = monitored_items_.begin(); i != monitored_items_.end();) {
    auto* monitored_item = *i++;
    if (monitored_item->subscribed())
      monitored_item->ForwardData(value);
  }
}

void VariableHandle::UpdateQualifier(unsigned remove, unsigned add) {
  if (last_value_.is_null()) {
    auto timestamp = DateTime::Now();
    ForwardData({scada::Variant{}, Qualifier{add}, timestamp, timestamp});
    return;
  }

  Qualifier new_qualifier = last_value_.qualifier;
  new_qualifier.Update(remove, add);
  if (last_value_.qualifier == new_qualifier)
    return;

  DataValue data = last_value_;
  data.qualifier = new_qualifier;

  ForwardData(data);
}

void VariableHandle::Deleted() {
  UpdateQualifier(0, Qualifier::FAILED);
}

void VariableHandle::Write(
    const std::shared_ptr<const scada::ServiceContext>& context,
    const scada::WriteValue& input,
    const scada::StatusCallback& callback) {
  callback(StatusCode::Bad_WrongMethodId);
}

void VariableHandle::Call(const NodeId& method_id,
                          const std::vector<Variant>& arguments,
                          const scada::NodeId& user_id,
                          const StatusCallback& callback) {
  callback(StatusCode::Bad_WrongMethodId);
}

// static
std::shared_ptr<VariableHandleImpl> VariableHandleImpl::Create() {
  std::shared_ptr<VariableHandleImpl> result(new VariableHandleImpl);
  return result;
}

}  // namespace scada
