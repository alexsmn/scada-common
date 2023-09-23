#pragma once

#include "common/data_value_util.h"
#include "common/value_format.h"
#include "events/event_set.h"
#include "node_service/node_ref.h"
#include "scada/aggregate_filter.h"
#include "scada/data_value.h"
#include "scada/status.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_property.h"

#include <functional>
#include <memory>
#include <string>

class TimedDataService;

namespace scada {
class WriteFlags;
}

class TimedData;

class TimedDataSpec : private TimedDataObserver {
 public:
  TimedDataSpec();
  TimedDataSpec(const TimedDataSpec& other);
  TimedDataSpec(std::shared_ptr<TimedData> data);
  TimedDataSpec(TimedDataService& service, std::string_view formula);
  TimedDataSpec(TimedDataService& service, const scada::NodeId& node_id);
  ~TimedDataSpec();

  void SetAggregateFilter(scada::AggregateFilter filter);

  // Specify |kTimedDataCurrentOnly| to get only current values.
  void SetFrom(base::Time from);
  void SetRange(const scada::DateTimeRange& range);

  void Connect(TimedDataService& service, std::string_view formula);
  void Connect(TimedDataService& service, const scada::NodeId& node_id);
  void Connect(TimedDataService& service, const NodeRef& node);

  std::string formula() const;
  bool alerting() const;
  bool connected() const { return data_ && !data_->IsError(); }
  base::Time from() const { return range_.first; }
  const scada::DateTimeRange& range() const { return range_; }
  base::Time ready_from() const;
  bool historical() const;
  bool logical() const;
  bool ready() const;  // connected and all requested data was received
  bool range_ready(const scada::DateTimeRange& range) const;
  scada::DataValue current() const;
  base::Time change_time() const;

  // Historical data.
  const DataValues* values() const;
  scada::DataValue GetValueAt(base::Time time) const;

  NodeRef GetNode() const;

  std::u16string GetCurrentString(const ValueFormat& format = ValueFormat{
                                      FORMAT_QUALITY | FORMAT_UNITS}) const;
  std::u16string GetValueString(const scada::Variant& value,
                                scada::Qualifier qualifier,
                                const ValueFormat& format = ValueFormat{
                                    FORMAT_QUALITY | FORMAT_UNITS}) const;
  std::u16string GetTitle() const;
  const EventSet* GetEvents() const;

  void Reset() { SetData(nullptr); }

  // Acknowledge all active events related to this data.
  void Acknowledge() const;

  using StatusCallback = std::function<void(const scada::Status&)>;
  void Write(double value,
             const scada::NodeId& user_id,
             const scada::WriteFlags& flags,
             const StatusCallback& callback) const;
  void Call(const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const scada::NodeId& user_id,
            const StatusCallback& callback) const;

  std::string DumpDebugInfo() const;

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

  // TimedDataObserver
  virtual void OnTimedDataCorrections(size_t count,
                                      const scada::DataValue* tvqs) final;
  virtual void OnTimedDataReady() final;
  virtual void OnTimedDataNodeModified() final;
  virtual void OnTimedDataDeleted() final;
  virtual void OnEventsChanged();
  virtual void OnPropertyChanged(const PropertySet& properties) final;

  std::shared_ptr<TimedData> data_;

  scada::AggregateFilter aggregate_filter_;
  scada::DateTimeRange range_{kTimedDataCurrentOnly, kTimedDataCurrentOnly};
};
