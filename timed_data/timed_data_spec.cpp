#include "timed_data/timed_data_spec.h"

#include "model/data_items_node_ids.h"
#include "node_service/node_format.h"
#include "node_service/node_util.h"
#include "timed_data/timed_data.h"
#include "timed_data/timed_data_service.h"
#include "timed_data/timed_data_util.h"

#include <algorithm>

TimedDataSpec::TimedDataSpec() = default;

TimedDataSpec::TimedDataSpec(const TimedDataSpec& other)
    : data_{other.data_}, range_{other.range_} {
  if (data_) {
    data_->AddObserver(*this);
    data_->AddViewObserver(*this, range_);
  }
}

TimedDataSpec::TimedDataSpec(std::shared_ptr<TimedData> data)
    : data_(std::move(data)) {
  if (data_) {
    data_->AddObserver(*this);
    data_->AddViewObserver(*this, range_);
  }
}

TimedDataSpec::TimedDataSpec(TimedDataService& service,
                             std::string_view formula) {
  Connect(service, formula);
}

TimedDataSpec::TimedDataSpec(TimedDataService& service,
                             const scada::NodeId& node_id) {
  Connect(service, node_id);
}

TimedDataSpec::~TimedDataSpec() {
  Reset();
}

bool TimedDataSpec::connected() const {
  return data_ && !data_->IsError();
}

void TimedDataSpec::SetCurrentOnly() {
  SetFrom(kTimedDataCurrentOnly);
}

void TimedDataSpec::SetFrom(base::Time from) {
  SetRange({from, kTimedDataCurrentOnly});
}

void TimedDataSpec::SetRange(const scada::DateTimeRange& range) {
  assert(!range.second.is_null());
  assert(IsValidInterval(range));
  assert(range.second == kTimedDataCurrentOnly || !IsEmptyInterval(range));

  if (range_ == range)
    return;

  range_ = range;

  if (data_) {
    data_->AddObserver(*this);
    data_->AddViewObserver(*this, range_);
  }
}

void TimedDataSpec::SetAggregateFilter(scada::AggregateFilter filter) {
  assert(!data_);
  aggregate_filter_ = std::move(filter);
}

void TimedDataSpec::SetData(std::shared_ptr<TimedData> data) {
  if (data_ == data)
    return;

  if (data) {
    data->AddObserver(*this);
    data->AddViewObserver(*this, range_);
  }

  if (data_) {
    data_->RemoveObserver(*this);
    data_->RemoveViewObserver(*this);
  }

  data_ = data;
}

void TimedDataSpec::Connect(TimedDataService& service,
                            std::string_view formula) {
  SetData(service.GetFormulaTimedData(formula, aggregate_filter_));
}

void TimedDataSpec::Connect(TimedDataService& service,
                            const scada::NodeId& node_id) {
  SetData(service.GetNodeTimedData(node_id, aggregate_filter_));
}

void TimedDataSpec::Connect(TimedDataService& service, const NodeRef& node) {
  SetData(service.GetNodeTimedData(node.node_id(), aggregate_filter_));
}

std::u16string TimedDataSpec::GetCurrentString(
    const ValueFormat& format) const {
  auto value = current();
  return GetValueString(value.value, value.qualifier, format);
}

std::u16string TimedDataSpec::GetValueString(
    const scada::Variant& value,
    scada::Qualifier qualifier,
    const ValueFormat& format_options) const {
  if (auto node = this->node()) {
    return FormatValue(node, value, qualifier, format_options.flags);
  }

  std::u16string string_value;
  value.get(string_value);
  return string_value;
}

bool TimedDataSpec::logical() const {
  return IsInstanceOf(node(), data_items::id::DiscreteItemType);
}

bool TimedDataSpec::ready() const {
  return range_ready(range_);
}

bool TimedDataSpec::range_ready(const scada::DateTimeRange& range) const {
  if (!data_)
    return true;

  const auto& ready_ranges = data_->GetReadyRanges();
  return IntervalsContain(ready_ranges, range);
}

TimedDataSpec& TimedDataSpec::operator=(const TimedDataSpec& other) {
  if (&other != this) {
    range_ = other.range_;
    SetData(other.data_);
  }
  return *this;
}

void TimedDataSpec::Acknowledge() const {
  if (data_)
    data_->Acknowledge();
}

bool TimedDataSpec::alerting() const {
  return data_ && data_->IsAlerting();
}

base::Time TimedDataSpec::ready_from() const {
  return data_ ? GetReadyFrom(data_->GetReadyRanges(), range_)
               : kTimedDataCurrentOnly;
}

scada::DataValue TimedDataSpec::current() const {
  return data_ ? data_->GetDataValue() : scada::DataValue();
}

base::Time TimedDataSpec::change_time() const {
  return data_ ? data_->GetChangeTime() : base::Time();
}

bool TimedDataSpec::historical() const {
  return range_.first != kTimedDataCurrentOnly;
}

const std::vector<scada::DataValue>* TimedDataSpec::values() const {
  return data_ ? data_->GetValues() : nullptr;
}

scada::NodeId TimedDataSpec::node_id() const {
  return data_ ? data_->GetNode().node_id() : scada::NodeId{};
}

NodeRef TimedDataSpec::node() const {
  return data_ ? data_->GetNode() : NodeRef{};
}

scada::node TimedDataSpec::scada_node() const {
  return data_ ? data_->GetNode().scada_node() : scada::node{};
}

scada::LocalizedText TimedDataSpec::GetTitle() const {
  return data_ ? data_->GetTitle() : scada::LocalizedText{kUnknownDisplayName};
}

scada::DataValue TimedDataSpec::GetValueAt(base::Time time) const {
  return data_ ? data_->GetValueAt(time) : scada::DataValue{};
}

const EventSet* TimedDataSpec::GetEvents() const {
  return data_ ? data_->GetEvents() : nullptr;
}

bool TimedDataSpec::operator==(const TimedDataSpec& other) const {
  return data_ == other.data_;
}

std::string TimedDataSpec::formula() const {
  return data_ ? data_->GetFormula(false) : std::string{};
}

void TimedDataSpec::OnTimedDataCorrections(size_t count,
                                           const scada::DataValue* values) {
  if (!correction_handler)
    return;

  auto data_values = std::span<const scada::DataValue>{values, count};
  auto start = LowerBound(data_values, range_.first);
  auto end = UpperBound(data_values, range_.second);

  if (start != end)
    correction_handler(end - start, values + start);
}

void TimedDataSpec::OnTimedDataReady() {
  if (ready_handler)
    ready_handler();
}

void TimedDataSpec::OnTimedDataNodeModified() {
  if (node_modified_handler)
    node_modified_handler();
}

void TimedDataSpec::OnTimedDataDeleted() {
  if (deletion_handler)
    deletion_handler();
}

void TimedDataSpec::OnEventsChanged() {
  if (event_change_handler)
    event_change_handler();
}

void TimedDataSpec::OnPropertyChanged(const PropertySet& properties) {
  if (property_change_handler)
    property_change_handler(properties);
}

std::string TimedDataSpec::DumpDebugInfo() const {
  return data_ ? data_->DumpDebugInfo() : std::string{};
}

// static
base::Time TimedDataSpec::GetTimedDataCurrentOnly() {
  return kTimedDataCurrentOnly;
}