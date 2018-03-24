#pragma once

#include <memory>
#include <set>

#include "core/data_value.h"
#include "core/monitored_item.h"

namespace scada {

class VariableHandle;
class WriteFlags;

class VariableMonitoredItem : public MonitoredItem {
 public:
  explicit VariableMonitoredItem(std::shared_ptr<VariableHandle> variable);
  virtual ~VariableMonitoredItem();

  bool subscribed() const { return subscribed_; }

  // MonitoredItem
  virtual void Subscribe() override;

 private:
  const std::shared_ptr<VariableHandle> variable_;
  bool subscribed_ = false;
};

using StatusCallback = std::function<void(const Status& status)>;

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

  virtual void Write(double value,
                     const NodeId& user_id,
                     const WriteFlags& flags,
                     const StatusCallback& callback);

  virtual void Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback);

 private:
  friend class VariableMonitoredItem;

  DataValue last_value_;
  base::Time last_change_time_;

  std::set<VariableMonitoredItem*> monitored_items_;
};

std::unique_ptr<VariableMonitoredItem> CreateMonitoredVariable(
    std::shared_ptr<VariableHandle> variable);

}  // namespace scada
