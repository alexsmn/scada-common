#pragma once

#include <functional>
#include <memory>

#include "base/strings/string16.h"
#include "core/configuration_types.h"
#include "core/status.h"
#include "common/format.h"
#include "core/data_value.h"
#include "common/timed_data/timed_vq_map.h"
#include "common/timed_data/timed_data_delegate.h"
#include "common/event_set.h"
#include "common/node_ref.h"

class TimedDataService;

namespace scada {
class WriteFlags;
}

namespace rt {

class TimedData;

class TimedDataSpec {
 public:
  TimedDataSpec();
  TimedDataSpec(const TimedDataSpec& other);
  explicit TimedDataSpec(std::shared_ptr<TimedData> data);
  ~TimedDataSpec();

  void set_delegate(TimedDataDelegate* delegate) { delegate_ = delegate; }

  // Specify |kTimedDataCurrentOnly| to get only current values.
  void SetFrom(base::Time from);

  TimedDataDelegate* delegate() const { return delegate_; }

  const std::string& formula() const { return formula_; }
  bool alerting() const;
  bool connected() const { return !!data_; }
  base::Time from() const { return from_; }
  base::Time ready_from() const;
  bool historical() const;
  bool logical() const;
  bool ready() const;	// connected and all requested data was received
  scada::DataValue current() const;
  base::Time change_time() const;

  // Historical data.
  const TimedVQMap* values() const;

  // TODO: Rename to node_id().
  scada::NodeId trid() const;

  NodeRef GetNode() const;

  base::string16 GetCurrentString(int params = FORMAT_QUALITY | FORMAT_UNITS) const;
  base::string16 GetValueString(const scada::Variant& value, scada::Qualifier qualifier,
                                int params = FORMAT_QUALITY | FORMAT_UNITS) const;
  base::string16 GetTitle() const;
  scada::DataValue GetValueAt(const base::Time& time) const;
  const events::EventSet* GetEvents() const;

  void Connect(TimedDataService& timed_data_service, const std::string& formula);
  void Connect(TimedDataService& timed_data_service, const scada::NodeId& node_id);

  void Reset() { SetData(NULL, false); }

  // Acknowledge all active events related to this data.
  void Acknowledge() const;

  typedef std::function<void(const scada::Status&)> StatusCallback;
  void Write(double value, const scada::WriteFlags& flags, const StatusCallback& callback) const;
  void Call(const scada::NodeId& method_id, const std::vector<scada::Variant>& arguments,
            const StatusCallback& callback) const;
  
  TimedDataSpec& operator=(const TimedDataSpec& other);

  bool operator==(const TimedDataSpec& other) const;

  void* param;

private:
  friend class ExpressionTimedData;
  friend class TimedData;

  void SetData(std::shared_ptr<TimedData> data, bool update);

  // TODO: Move into TimedData.
  std::string formula_;

  base::Time from_;
  TimedDataDelegate* delegate_;

  std::shared_ptr<TimedData> data_;
};

} // namespace rt
