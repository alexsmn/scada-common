#pragma once

#include "common/value_format.h"
#include "node_service/node_ref.h"
#include "scada/aggregate_filter.h"
#include "scada/data_value.h"
#include "scada/date_time_range.h"
#include "scada/node.h"
#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_view_observer.h"

#include <functional>
#include <memory>
#include <string>

class EventSet;
class PropertySet;
class TimedData;
class TimedDataService;

class TimedDataSpec final : private TimedDataObserver,
                            private TimedDataViewObserver {
 public:
  TimedDataSpec();
  TimedDataSpec(const TimedDataSpec& other);
  TimedDataSpec(std::shared_ptr<TimedData> data);
  TimedDataSpec(TimedDataService& service, std::string_view formula);
  TimedDataSpec(TimedDataService& service, const scada::NodeId& node_id);
  ~TimedDataSpec();

  void SetAggregateFilter(scada::AggregateFilter filter);

  void SetCurrentOnly();
  void SetFrom(base::Time from);
  void SetRange(const scada::DateTimeRange& range);

  void Connect(TimedDataService& service, std::string_view formula);
  void Connect(TimedDataService& service, const scada::NodeId& node_id);
  void Connect(TimedDataService& service, const NodeRef& node);

  std::string formula() const;
  bool alerting() const;
  bool connected() const;
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
  std::span<const scada::DataValue> values() const noexcept;
  scada::DataValue GetValueAt(base::Time time) const;

  scada::NodeId node_id() const;
  NodeRef node() const;
  scada::node scada_node() const;

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

  std::string DumpDebugInfo() const;

  TimedDataSpec& operator=(const TimedDataSpec& other);

  bool operator==(const TimedDataSpec& other) const;

  std::function<void(std::span<const scada::DataValue> values)> update_handler;
  std::function<void()> ready_handler;
  std::function<void()> node_modified_handler;
  std::function<void()> deletion_handler;
  std::function<void()> event_change_handler;
  std::function<void(const PropertySet& properties)> property_change_handler;

 private:
  void SetData(std::shared_ptr<TimedData> data);

  // TimedDataObserver
  virtual void OnTimedDataNodeModified() override;
  virtual void OnTimedDataDeleted() override;
  virtual void OnEventsChanged() override;
  virtual void OnPropertyChanged(const PropertySet& properties) override;

  // TimedDataViewObserver
  virtual void OnTimedDataUpdates(
      std::span<const scada::DataValue> values) override;
  virtual void OnTimedDataReady() override;

  std::shared_ptr<TimedData> data_;

  scada::AggregateFilter aggregate_filter_;
  scada::DateTimeRange range_{GetTimedDataCurrentOnly(),
                              GetTimedDataCurrentOnly()};

  // Keep it defined in the source file to void heavy includes.
  static base::Time GetTimedDataCurrentOnly();
};
