#include "address_space/local_history_service.h"

#include "base/time/time.h"
#include "base/utf_convert.h"
#include "model/node_id_util.h"
#include "scada/data_value.h"
#include "scada/status.h"

#include <boost/json.hpp>

#include <cmath>
#include <random>

namespace scada {

namespace {

DataValue MakeValueAt(Variant value, base::Time time) {
  return DataValue{std::move(value), {}, time, time};
}

}  // namespace

LocalHistoryService::LocalHistoryService() = default;
LocalHistoryService::~LocalHistoryService() = default;

void LocalHistoryService::SetRawProfile(const NodeId& node_id,
                                        double base_value) {
  raw_profiles_[node_id] = base_value;
}

void LocalHistoryService::AddEvent(Event event) {
  events_.push_back(std::move(event));
}

UInt32 LocalHistoryService::ParseSeverity(std::string_view s) {
  if (s == "warning")
    return kSeverityWarning;
  if (s == "critical")
    return kSeverityCritical;
  return kSeverityNormal;
}

void LocalHistoryService::LoadFromJson(const boost::json::value& root) {
  // Raw-history base values from the `nodes` array.
  for (const auto& jn : root.at("nodes").as_array()) {
    if (auto* bv = jn.as_object().if_contains("base_value")) {
      SetRawProfile(
          NodeIdFromScadaString(std::string_view(jn.at("id").as_string())),
          bv->to_number<double>());
    }
  }

  // Events from the `events` array.
  const auto now = base::Time::Now();
  for (const auto& je : root.at("events").as_array()) {
    Event e;
    e.event_id = static_cast<EventId>(je.at("id").as_int64());
    double hours_ago = je.at("hours_ago").to_number<double>();
    e.time = now - base::TimeDelta::FromSecondsD(hours_ago * 3600);
    e.receive_time = e.time;
    e.severity = ParseSeverity(je.at("severity").as_string());
    e.message = LocalizedText{
        UtfConvert<char16_t>(std::string(je.at("message").as_string()))};
    e.node_id =
        NodeIdFromScadaString(std::string_view(je.at("node_id").as_string()));
    e.change_mask = static_cast<UInt32>(je.at("change_mask").as_int64());
    e.acked = true;
    e.acknowledged_time = e.time;
    AddEvent(std::move(e));
  }
}

void LocalHistoryService::HistoryReadRaw(
    const HistoryReadRawDetails& details,
    const HistoryReadRawCallback& callback) {
  const auto now = base::Time::Now();

  double base_value = 100.0;
  if (auto it = raw_profiles_.find(details.node_id); it != raw_profiles_.end())
    base_value = it->second;

  // Deterministic per-node noise: seed on the numeric id so output is stable
  // across runs for a given node.
  auto seed = details.node_id.is_numeric() ? details.node_id.numeric_id() : 0u;
  std::mt19937 rng{seed};
  std::normal_distribution<double> dist(base_value, std::abs(base_value) * 0.05);

  std::vector<DataValue> values;
  values.reserve(48);
  for (int i = 0; i < 48; ++i) {
    auto time = now - base::TimeDelta::FromMinutes(30 * (48 - i));
    values.push_back(MakeValueAt(Variant{dist(rng)}, time));
  }

  callback(HistoryReadRawResult{
      .status = Status{StatusCode::Good},
      .values = std::move(values),
  });
}

void LocalHistoryService::HistoryReadEvents(
    const NodeId& /*node_id*/,
    base::Time /*from*/,
    base::Time /*to*/,
    const EventFilter& /*filter*/,
    const HistoryReadEventsCallback& callback) {
  callback(HistoryReadEventsResult{
      .status = Status{StatusCode::Good},
      .events = events_,
  });
}

}  // namespace scada
