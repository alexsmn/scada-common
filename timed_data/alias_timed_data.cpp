#include "alias_timed_data.h"

AliasTimedData::AliasTimedData(std::string formula)
    : data_{std::make_unique<DeferredData>(std::move(formula))} {}

void AliasTimedData::SetForwarded(std::shared_ptr<TimedData> timed_data) {
  assert(!is_forwarded());
  assert(timed_data);

  auto deferred = std::move(std::get<std::unique_ptr<DeferredData>>(data_));

  data_ = timed_data;

  for (auto [observer, range] : deferred->observers)
    timed_data->AddObserver(*observer, range);

  for (auto [observer, range] : deferred->observers) {
    observer->OnPropertyChanged(
        PropertySet{PROPERTY_TITLE | PROPERTY_ITEM | PROPERTY_CURRENT});
    observer->OnTimedDataNodeModified();
    observer->OnEventsChanged();
  }
}

bool AliasTimedData::IsError() const {
  return is_forwarded() && forwarded().IsError();
}

const std::vector<scada::DateTimeRange>& AliasTimedData::GetReadyRanges()
    const {
  return is_forwarded() ? forwarded().GetReadyRanges() : kReadyCurrentTimeOnly;
}

scada::DataValue AliasTimedData::GetDataValue() const {
  return is_forwarded() ? forwarded().GetDataValue() : scada::DataValue{};
}
scada::DataValue AliasTimedData::GetValueAt(const base::Time& time) const {
  return is_forwarded() ? forwarded().GetValueAt(time) : scada::DataValue{};
}

base::Time AliasTimedData::GetChangeTime() const {
  return is_forwarded() ? forwarded().GetChangeTime() : base::Time{};
}

const DataValues* AliasTimedData::GetValues() const {
  return is_forwarded() ? forwarded().GetValues() : nullptr;
}

void AliasTimedData::AddObserver(TimedDataObserver& observer,
                                 const scada::DateTimeRange& range) {
  if (is_forwarded())
    forwarded().AddObserver(observer, range);
  else
    deferred().observers.insert_or_assign(&observer, range);
}

void AliasTimedData::RemoveObserver(TimedDataObserver& observer) {
  if (is_forwarded())
    forwarded().RemoveObserver(observer);
  else
    deferred().observers.erase(&observer);
}

std::string AliasTimedData::GetFormula(bool aliases) const {
  return is_forwarded() ? forwarded().GetFormula(aliases) : deferred().formula;
}

scada::LocalizedText AliasTimedData::GetTitle() const {
  return is_forwarded() ? forwarded().GetTitle()
                        : scada::ToLocalizedText(deferred().formula);
}

NodeRef AliasTimedData::GetNode() const {
  return is_forwarded() ? forwarded().GetNode() : NodeRef{};
}

bool AliasTimedData::IsAlerting() const {
  return is_forwarded() ? forwarded().IsAlerting() : false;
}

const EventSet* AliasTimedData::GetEvents() const {
  return is_forwarded() ? forwarded().GetEvents() : nullptr;
}

void AliasTimedData::Acknowledge() {
  if (is_forwarded())
    forwarded().Acknowledge();
}

void AliasTimedData::Write(double value,
                           const scada::NodeId& user_id,
                           const scada::WriteFlags& flags,
                           const StatusCallback& callback) const {
  if (is_forwarded())
    forwarded().Write(value, user_id, flags, callback);
  else
    callback(scada::StatusCode::Bad_Disconnected);
}

void AliasTimedData::Call(const scada::NodeId& method_id,
                          const std::vector<scada::Variant>& arguments,
                          const scada::NodeId& user_id,
                          const StatusCallback& callback) const {
  if (is_forwarded())
    forwarded().Call(method_id, arguments, user_id, callback);
  else
    callback(scada::StatusCode::Bad_Disconnected);
}

std::string AliasTimedData::DumpDebugInfo() const {
  std::string result = "AliasTimedData\n";
  if (is_forwarded())
    result += forwarded().DumpDebugInfo();
  return result;
}
