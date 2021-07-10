#pragma once

#include "core/attribute_ids.h"
#include "core/attribute_service.h"
#include "core/data_value.h"
#include "core/monitored_item.h"

#include <memory>
#include <set>

namespace scada {

class VariableHandle;
class WriteFlags;

class VariableMonitoredItem : public MonitoredItem {
 public:
  explicit VariableMonitoredItem(std::shared_ptr<VariableHandle> variable);
  virtual ~VariableMonitoredItem();

  const scada::DataChangeHandler& data_change_handler() const {
    return data_change_handler_;
  }

  // MonitoredItem
  virtual void Subscribe(scada::MonitoredItemHandler handler) override;

 private:
  const std::shared_ptr<VariableHandle> variable_;

  scada::DataChangeHandler data_change_handler_;
};

class VariableHandle : public std::enable_shared_from_this<VariableHandle> {
 public:
  virtual ~VariableHandle();

  void set_last_value(DataValue value) { last_value_ = std::move(value); }
  void set_last_change_time(DateTime time) { last_change_time_ = time; }

  void ForwardData(const DataValue& value);
  void UpdateQualifier(unsigned remove, unsigned add);

  void Deleted();

  const DataValue& last_value() const { return last_value_; }
  DateTime last_change_time() const { return last_change_time_; }

  virtual void Write(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const scada::WriteValue& input,
      const scada::StatusCallback& callback);

  virtual void Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const NodeId& user_id,
                    const StatusCallback& callback);

 protected:
  VariableHandle() {}

 private:
  friend class VariableMonitoredItem;

  DataValue last_value_;
  base::Time last_change_time_;

  std::set<VariableMonitoredItem*> monitored_items_;
};

class VariableHandleImpl : public VariableHandle {
 public:
  static std::shared_ptr<VariableHandleImpl> Create();
};

std::shared_ptr<VariableMonitoredItem> CreateMonitoredVariable(
    std::shared_ptr<VariableHandle> variable);

}  // namespace scada
