#include "timed_data/timed_data_spec.h"

#include "base/strings/utf_string_conversions.h"
#include "common/node_format.h"
#include "common/node_util.h"
#include "common/scada_node_ids.h"
#include "timed_data/timed_data_service.h"

#include <algorithm>

namespace rt {

TimedDataSpec::TimedDataSpec() : from_(kTimedDataCurrentOnly) {}

TimedDataSpec::TimedDataSpec(const TimedDataSpec& other)
    : data_{other.data_}, from_{other.from_} {}

TimedDataSpec::TimedDataSpec(std::shared_ptr<TimedData> data)
    : data_(std::move(data)), from_(kTimedDataCurrentOnly) {
  if (data_)
    data_->AddObserver(*this);
}

TimedDataSpec::~TimedDataSpec() {
  Reset();
}

void TimedDataSpec::SetFrom(base::Time from) {
  from_ = from;
  if (data_)
    data_->SetFrom(from);
}

void TimedDataSpec::SetData(std::shared_ptr<TimedData> data) {
  if (data_ == data)
    return;

  if (data) {
    data->AddObserver(*this);
    data->SetFrom(from_);
  }

  if (data_)
    data_->RemoveObserver(*this);

  data_ = data;
}

void TimedDataSpec::Connect(TimedDataService& service,
                            base::StringPiece formula) {
  SetData(service.GetFormulaTimedData(formula));
}

void TimedDataSpec::Connect(TimedDataService& service,
                            const scada::NodeId& node_id) {
  SetData(service.GetNodeTimedData(node_id));
}

void TimedDataSpec::Connect(TimedDataService& service, const NodeRef& node) {
  SetData(service.GetNodeTimedData(node.node_id()));
}

base::string16 TimedDataSpec::GetCurrentString(int params) const {
  auto value = current();
  return GetValueString(value.value, value.qualifier, params);
}

base::string16 TimedDataSpec::GetValueString(const scada::Variant& value,
                                             scada::Qualifier qualifier,
                                             int params) const {
  if (data_) {
    if (auto node = GetNode())
      return FormatValue(node, value, qualifier, params);
  }

  base::string16 string_value;
  value.get(string_value);
  return string_value;
}

bool TimedDataSpec::logical() const {
  return IsInstanceOf(GetNode(), id::DiscreteItemType);
}

bool TimedDataSpec::ready() const {
  return data_ ? data_->GetReadyFrom() <= from_ : true;
}

TimedDataSpec& TimedDataSpec::operator=(const TimedDataSpec& other) {
  if (&other != this) {
    from_ = other.from_;
    SetData(other.data_);
  }
  return *this;
}

void TimedDataSpec::Write(double value,
                          const scada::NodeId& user_id,
                          const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  if (!data_) {
    if (callback)
      callback(scada::StatusCode::Bad_Disconnected);
    return;
  }

  return data_->Write(value, user_id, flags, callback);
}

void TimedDataSpec::Call(const scada::NodeId& method_id,
                         const std::vector<scada::Variant>& arguments,
                         const scada::NodeId& user_id,
                         const StatusCallback& callback) const {
  if (!data_) {
    if (callback)
      callback(scada::StatusCode::Bad_Disconnected);
    return;
  }

  return data_->Call(method_id, arguments, user_id, callback);
}

void TimedDataSpec::Acknowledge() const {
  if (data_)
    data_->Acknowledge();
}

bool TimedDataSpec::alerting() const {
  return data_ && data_->IsAlerting();
}

base::Time TimedDataSpec::ready_from() const {
  return data_ ? data_->GetReadyFrom() : kTimedDataCurrentOnly;
}

scada::DataValue TimedDataSpec::current() const {
  return data_ ? data_->GetDataValue() : scada::DataValue();
}

base::Time TimedDataSpec::change_time() const {
  return data_ ? data_->GetChangeTime() : base::Time();
}

bool TimedDataSpec::historical() const {
  return from_ != kTimedDataCurrentOnly;
}

const TimedVQMap* TimedDataSpec::values() const {
  return data_ ? data_->GetValues() : nullptr;
}

NodeRef TimedDataSpec::GetNode() const {
  return data_ ? data_->GetNode() : NodeRef{};
}

base::string16 TimedDataSpec::GetTitle() const {
  return data_ ? data_->GetTitle() : base::WideToUTF16(kUnknownDisplayName);
}

scada::DataValue TimedDataSpec::GetValueAt(base::Time time) const {
  return data_ ? data_->GetValueAt(time) : scada::DataValue{};
}

const events::EventSet* TimedDataSpec::GetEvents() const {
  return data_ ? data_->GetEvents() : nullptr;
}

bool TimedDataSpec::operator==(const TimedDataSpec& other) const {
  return data_ == other.data_;
}

std::string TimedDataSpec::formula() const {
  return data_ ? data_->GetFormula(false) : std::string{};
}

void TimedDataSpec::OnTimedDataCorrections(size_t count,
                                           const scada::DataValue* tvqs) {
  if (!correction_handler)
    return;

  auto end =
      std::upper_bound(tvqs, tvqs + count, from_,
                       [](base::Time from, const scada::DataValue& value) {
                         return from < value.source_timestamp;
                       });
  size_t partial_count = end - tvqs;
  if (partial_count != 0)
    correction_handler(partial_count, tvqs);
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

}  // namespace rt
