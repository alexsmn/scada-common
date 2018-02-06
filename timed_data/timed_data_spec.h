#pragma once

#include <functional>
#include <memory>

#include "base/strings/string16.h"
#include "common/event_set.h"
#include "common/format.h"
#include "common/node_ref.h"
#include "core/configuration_types.h"
#include "core/data_value.h"
#include "core/status.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_delegate.h"
#include "timed_data/timed_vq_map.h"

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

  void Connect(TimedDataService& service, const std::string& formula);
  void Connect(TimedDataService& service, const scada::NodeId& node_id);

  TimedDataDelegate* delegate() const { return delegate_; }

  const std::string& formula() const { return formula_; }
  bool alerting() const;
  bool connected() const { return !!data_; }
  base::Time from() const { return from_; }
  base::Time ready_from() const;
  bool historical() const;
  bool logical() const;
  bool ready() const;  // connected and all requested data was received
  scada::DataValue current() const;
  base::Time change_time() const;

  // Historical data.
  const TimedVQMap* values() const;
  scada::DataValue GetValueAt(base::Time time) const;

  NodeRef GetNode() const;

  base::string16 GetCurrentString(int params = FORMAT_QUALITY |
                                               FORMAT_UNITS) const;
  base::string16 GetValueString(const scada::Variant& value,
                                scada::Qualifier qualifier,
                                int params = FORMAT_QUALITY |
                                             FORMAT_UNITS) const;
  base::string16 GetTitle() const;
  const events::EventSet* GetEvents() const;

  void Reset() { SetData(NULL, false); }

  // Acknowledge all active events related to this data.
  void Acknowledge() const;

  typedef std::function<void(const scada::Status&)> StatusCallback;
  void Write(double value,
             const scada::WriteFlags& flags,
             const StatusCallback& callback) const;
  void Call(const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const StatusCallback& callback) const;

  TimedDataSpec& operator=(const TimedDataSpec& other);

  bool operator==(const TimedDataSpec& other) const;

  void* param;

 private:
  friend class ExpressionTimedData;
  friend class TimedData;

  void SetData(std::shared_ptr<TimedData> data, bool update);

  std::shared_ptr<TimedData> data_;

  base::Time from_ = kTimedDataCurrentOnly;

  // TODO: Move into TimedData.
  std::string formula_;

  TimedDataDelegate* delegate_ = nullptr;
};

}  // namespace rt
