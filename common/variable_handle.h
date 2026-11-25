#pragma once

#include "base/lifetime.h"
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
  void set_last_change_time(Time time) { last_change_time_ = time; }

  void ForwardData(const DataValue& value);
  void UpdateQualifier(unsigned remove, unsigned add);

  void Deleted();

  const DataValue& last_value() const SCADA_LIFETIME_BOUND {
    return last_value_;
  }
  Time last_change_time() const { return last_change_time_; }

  // Reflects `scada::DataChangeHandler` signature.
  using DataChangeSignal =
      boost::signals2::signal<void(const DataValue& data_value)>;
  DataChangeSignal& data_change_signal() SCADA_LIFETIME_BOUND {
    return data_change_signal_;
  }

  virtual void Write(const ServiceContext& context,
                     const WriteValue& input,
                     const StatusCallback& callback);

  // Takes the whole ServiceContext, like Write above. It used to take a bare
  // user id, which silently dropped the caller's rights bitmask — any write a
  // method performed downstream was then re-checked against empty rights and
  // denied.
  virtual void Call(const ServiceContext& context,
                    const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const StatusCallback& callback);

 private:
  DataValue last_value_;
  scada::Time last_change_time_ = scada::kNullTime;

  DataChangeSignal data_change_signal_;
};

class VariableHandleImpl : public VariableHandle {
 public:
  static std::shared_ptr<VariableHandleImpl> Create();
};

// A monitored item on |variable|: Subscribe reports the variable's current
// value when it has one, then every change. A variable that has never
// received data reports nothing until it does.
std::shared_ptr<MonitoredItem> CreateMonitoredVariable(
    std::shared_ptr<VariableHandle> variable);

// The same, for an item a client monitors: Subscribe always reports a first
// sample, as OPC UA Part 4 §5.13.1.3 Monitoring mode requires ("the Server
// shall report the first sample as soon as possible",
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.13.1.3). A
// variable that has never received data reports Bad_WaitingForInitialData
// with no value and no source timestamp, so nothing that keeps history
// records it. Kept apart from CreateMonitoredVariable because the device and
// formula chains inside the server must not see a sample nobody produced.
std::shared_ptr<MonitoredItem> CreateClientMonitoredVariable(
    std::shared_ptr<VariableHandle> variable);

}  // namespace scada
