#include "timed_data/alias_timed_data.h"

#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_property.h"

AliasTimedData::AliasTimedData(std::string formula)
    : data_{std::make_unique<DeferredData>(std::move(formula))} {}

void AliasTimedData::SetForwarded(std::shared_ptr<TimedData> timed_data) {
  assert(!is_forwarded());
  assert(timed_data);

  auto deferred = std::move(std::get<std::unique_ptr<DeferredData>>(data_));

  data_ = timed_data;

  for (auto [observer, range] : deferred->view_observers)
    timed_data->AddViewObserver(*observer, range);

  for (auto* observer : deferred->observers) {
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

const std::vector<scada::DataValue>* AliasTimedData::GetValues() const {
  return is_forwarded() ? forwarded().GetValues() : nullptr;
}

void AliasTimedData::AddObserver(TimedDataObserver& observer) {
  if (is_forwarded())
    forwarded().AddObserver(observer);
  else
    deferred().observers.emplace(&observer);
}

void AliasTimedData::RemoveObserver(TimedDataObserver& observer) {
  if (is_forwarded())
    forwarded().RemoveObserver(observer);
  else
    deferred().observers.erase(&observer);
}

void AliasTimedData::AddViewObserver(TimedDataViewObserver& observer,
                                     const scada::DateTimeRange& range) {
  if (is_forwarded())
    forwarded().AddViewObserver(observer, range);
  else
    deferred().view_observers.insert_or_assign(&observer, range);
}

void AliasTimedData::RemoveViewObserver(TimedDataViewObserver& observer) {
  if (is_forwarded())
    forwarded().RemoveViewObserver(observer);
  else
    deferred().view_observers.erase(&observer);
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

std::string AliasTimedData::DumpDebugInfo() const {
  std::string result = "AliasTimedData\n";
  if (is_forwarded())
    result += forwarded().DumpDebugInfo();
  return result;
}
