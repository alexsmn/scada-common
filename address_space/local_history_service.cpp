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
                                        double base_value,
                                        std::optional<double> noise_stddev) {
  raw_profiles_[node_id] = RawProfile{base_value, noise_stddev};
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

void LocalHistoryService::SetNowOverride(base::Time now) {
  now_override_ = now;
}

base::Time LocalHistoryService::Now() const {
  return now_override_.is_null() ? base::Time::Now() : now_override_;
}

void LocalHistoryService::LoadFromJson(const boost::json::value& root) {
  // Optional frozen clock, so regenerating screenshots doesn't shift every
  // rendered timestamp.
  if (const auto* jnow = root.as_object().if_contains("now")) {
    base::Time now;
    if (base::Time::FromString(std::string(jnow->as_string()).c_str(), &now))
      SetNowOverride(now);
  }

  // Raw-history base values from the `nodes` array, plus an optional per-node
  // `history_stddev` that overrides the default noise amplitude.
  for (const auto& jn : root.at("nodes").as_array()) {
    if (auto* bv = jn.as_object().if_contains("base_value")) {
      std::optional<double> noise_stddev;
      if (auto* sd = jn.as_object().if_contains("history_stddev"))
        noise_stddev = sd->to_number<double>();
      SetRawProfile(
          NodeIdFromScadaString(std::string_view(jn.at("id").as_string())),
          bv->to_number<double>(), noise_stddev);
    }
  }

  // Events from the `events` array.
  const auto now = Now();
  for (const auto& je : root.at("events").as_array()) {
    Event e;
    e.event_id = static_cast<EventId>(je.at("id").as_int64());
    double hours_ago = je.at("hours_ago").to_number<double>();
    e.time = now - base::TimeDelta::FromSecondsD(hours_ago * 3600);
    e.receive_time = e.time;
    e.severity = ParseSeverity(je.at("severity").as_string());
    e.message = LocalizedText{
        UtfConvert<char16_t>(std::string(je.at("message").as_string()))};
    e.source_node_id =
        NodeIdFromScadaString(std::string_view(je.at("node_id").as_string()));
    e.change_mask = static_cast<UInt32>(je.at("change_mask").as_int64());
    // Acknowledged unless the entry says otherwise — an `"acknowledged":
    // false` event stays pending, so alarm-surface chrome (pending markers,
    // backlog summaries) has something real to render.
    e.acked = true;
    if (const auto* ja = je.as_object().if_contains("acknowledged"))
      e.acked = ja->as_bool();
    if (e.acked)
      e.acknowledged_time = e.time;
    AddEvent(std::move(e));
  }
}

Awaitable<HistoryReadRawResult> LocalHistoryService::HistoryReadRaw(
    HistoryReadRawDetails details) {
  co_return ReadRaw(std::move(details));
}

Awaitable<HistoryReadEventsResult> LocalHistoryService::HistoryReadEvents(
    NodeId node_id,
    base::Time from,
    base::Time to,
    EventFilter filter) {
  co_return ReadEvents(std::move(node_id), from, to, std::move(filter));
}

HistoryReadRawResult LocalHistoryService::ReadRaw(
    HistoryReadRawDetails details) const {
  // Anchor the synthesized series to the requested window when it has a
  // finite end: consumers (TimedDataFetcher) filter returned values by the
  // requested range, so a series pinned to a frozen "now" would otherwise
  // vanish for wall-clock-ranged queries, and vice versa.
  const auto now = details.to.is_null() ? Now() : details.to;

  // Span the requested window when it is finite so every consumer gets a
  // fully-populated series regardless of its range — a graph's 24 h span and
  // a table row's 1 h sparkline window both read 48 points (a 24 h request
  // keeps the historical 30-minute spacing exactly). An open-ended request
  // falls back to that 30-minute spacing.
  base::TimeDelta interval = base::TimeDelta::FromMinutes(30);
  if (!details.from.is_null() && details.from < now)
    interval = (now - details.from) / 48;

  double base_value = 100.0;
  std::optional<double> noise_stddev;
  if (auto it = raw_profiles_.find(details.node_id);
      it != raw_profiles_.end()) {
    base_value = it->second.base_value;
    noise_stddev = it->second.noise_stddev;
  }

  // Deterministic per-node noise: seed on the numeric id so output is stable
  // across runs for a given node. The amplitude defaults to 5% of the mean,
  // but a node may pin an absolute standard deviation (e.g. so a narrow-range
  // frequency series stays inside its engineering-unit band).
  auto seed = details.node_id.is_numeric() ? details.node_id.numeric_id() : 0u;
  std::mt19937 rng{seed};
  std::normal_distribution<double> dist(
      base_value, noise_stddev.value_or(std::abs(base_value) * 0.05));

  std::vector<DataValue> values;
  values.reserve(48);
  for (int i = 0; i < 48; ++i) {
    auto time = now - interval * (48 - i);
    values.push_back(MakeValueAt(Variant{dist(rng)}, time));
  }

  return HistoryReadRawResult{
      .status = Status{StatusCode::Good},
      .values = std::move(values),
  };
}

HistoryReadEventsResult LocalHistoryService::ReadEvents(
    NodeId /*node_id*/,
    base::Time /*from*/,
    base::Time /*to*/,
    EventFilter /*filter*/) const {
  return HistoryReadEventsResult{
      .status = Status{StatusCode::Good},
      .events = events_,
  };
}

}  // namespace scada
