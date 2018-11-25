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
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_vq_map.h"

class TimedDataService;

namespace scada {
class WriteFlags;
}

namespace rt {

class TimedData;

class TimedDataSpec : private TimedDataDelegate {
 public:
  TimedDataSpec();
  TimedDataSpec(const TimedDataSpec& other);
  TimedDataSpec(std::shared_ptr<TimedData> data);
  ~TimedDataSpec();

  // Specify |kTimedDataCurrentOnly| to get only current values.
  void SetFrom(base::Time from);

  void Connect(TimedDataService& service, base::StringPiece formula);
  void Connect(TimedDataService& service, const scada::NodeId& node_id);
  void Connect(TimedDataService& service, const NodeRef& node);

  std::string formula() const;
  bool alerting() const;
  bool connected() const { return data_ && !data_->IsError(); }
  base::Time from() const { return from_; }
  base::Time ready_from() const;
  bool historical() const;
  bool logical() const;
  bool ready() const;  // connected and all requested data was received
  scada::DataValue current() const;
  base::Time change_time() const;

  // Historical data.
  const DataValues* values() const;
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

  void Reset() { SetData(nullptr); }

  // Acknowledge all active events related to this data.
  void Acknowledge() const;

  typedef std::function<void(const scada::Status&)> StatusCallback;
  void Write(double value,
             const scada::NodeId& user_id,
             const scada::WriteFlags& flags,
             const StatusCallback& callback) const;
  void Call(const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const scada::NodeId& user_id,
            const StatusCallback& callback) const;

  TimedDataSpec& operator=(const TimedDataSpec& other);

  bool operator==(const TimedDataSpec& other) const;

  std::function<void(size_t count, const scada::DataValue* tvqs)>
      correction_handler;
  std::function<void()> ready_handler;
  std::function<void()> node_modified_handler;
  std::function<void()> deletion_handler;
  std::function<void()> event_change_handler;
  std::function<void(const PropertySet& properties)> property_change_handler;

 private:
  void SetData(std::shared_ptr<TimedData> data);

  // TimedDataDelegate
  virtual void OnTimedDataCorrections(size_t count,
                                      const scada::DataValue* tvqs) final;
  virtual void OnTimedDataReady() final;
  virtual void OnTimedDataNodeModified() final;
  virtual void OnTimedDataDeleted() final;
  virtual void OnEventsChanged();
  virtual void OnPropertyChanged(const PropertySet& properties) final;

  std::shared_ptr<TimedData> data_;

  base::Time from_ = kTimedDataCurrentOnly;
};

}  // namespace rt
