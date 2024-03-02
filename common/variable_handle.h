#pragma once

#include "scada/attribute_ids.h"
#include "scada/attribute_service.h"
#include "scada/data_value.h"
#include "scada/monitored_item.h"

#include <boost/signals2/signal.hpp>
#include <memory>

namespace scada {

class VariableHandle;
class WriteFlags;

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

  // Reflects `scada::DataChangeHandler` signature.
  using DataChangeSignal =
      boost::signals2::signal<void(const DataValue& data_value)>;
  DataChangeSignal& data_change_signal() { return data_change_signal_; }

  virtual void Write(const ServiceContext& context,
                     const WriteValue& input,
                     const StatusCallback& callback);

  virtual void Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const NodeId& user_id,
                    const StatusCallback& callback);

 private:
  DataValue last_value_;
  base::Time last_change_time_;

  DataChangeSignal data_change_signal_;
};

class VariableHandleImpl : public VariableHandle {
 public:
  static std::shared_ptr<VariableHandleImpl> Create();
};

std::shared_ptr<MonitoredItem> CreateMonitoredVariable(
    std::shared_ptr<VariableHandle> variable);

}  // namespace scada
