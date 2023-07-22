#include "common/variable_handle.h"

#include "core/data_value.h"

namespace scada {

namespace {

class VariableMonitoredItem : public MonitoredItem {
 public:
  explicit VariableMonitoredItem(std::shared_ptr<VariableHandle> variable);

  // MonitoredItem
  virtual void Subscribe(MonitoredItemHandler handler) override;

 private:
  const std::shared_ptr<VariableHandle> variable_;

  DataChangeHandler data_change_handler_;

  boost::signals2::scoped_connection variable_connection_;
};

}  // namespace

std::shared_ptr<MonitoredItem> CreateMonitoredVariable(
    std::shared_ptr<VariableHandle> variable) {
  if (!variable)
    return nullptr;
  return std::make_shared<VariableMonitoredItem>(std::move(variable));
}

// VariableMonitoredItem

VariableMonitoredItem::VariableMonitoredItem(
    std::shared_ptr<VariableHandle> variable)
    : variable_(std::move(variable)) {}

void VariableMonitoredItem::Subscribe(MonitoredItemHandler handler) {
  assert(!data_change_handler_);

  data_change_handler_ = std::move(std::get<DataChangeHandler>(handler));

  variable_connection_ =
      variable_->data_change_signal().connect(data_change_handler_);

  if (!variable_->last_value().server_timestamp.is_null())
    data_change_handler_(variable_->last_value());
}

// VariableHandle

VariableHandle::~VariableHandle() = default;

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

  data_change_signal_(value);
}

void VariableHandle::UpdateQualifier(unsigned remove, unsigned add) {
  // |SourceTimestamp| is not updated, because next device value with a valid
  // |SourceTimestamp| overwrites this value.
  auto server_timestamp = DateTime::Now();

  if (last_value_.is_null()) {
    ForwardData({Variant{}, Qualifier{add}, DateTime{}, server_timestamp});
    return;
  }

  Qualifier new_qualifier = last_value_.qualifier;
  new_qualifier.Update(remove, add);
  if (last_value_.qualifier == new_qualifier)
    return;

  DataValue data = last_value_;
  data.qualifier = new_qualifier;
  data.server_timestamp = server_timestamp;

  ForwardData(data);
}

void VariableHandle::Deleted() {
  UpdateQualifier(0, Qualifier::FAILED);
}

void VariableHandle::Write(const std::shared_ptr<const ServiceContext>& context,
                           const WriteValue& input,
                           const StatusCallback& callback) {
  callback(StatusCode::Bad_WrongMethodId);
}

void VariableHandle::Call(const NodeId& method_id,
                          const std::vector<Variant>& arguments,
                          const NodeId& user_id,
                          const StatusCallback& callback) {
  callback(StatusCode::Bad_WrongMethodId);
}

// static
std::shared_ptr<VariableHandleImpl> VariableHandleImpl::Create() {
  std::shared_ptr<VariableHandleImpl> result(new VariableHandleImpl);
  return result;
}

}  // namespace scada
