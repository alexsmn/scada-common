#include "address_space/local_history_service.h"

#include "base/time/calendar.h"
#include "base/time/time.h"
#include "scada/date_time.h"

#include "base/utf_convert.h"
#include "model/node_id_util.h"
#include "scada/co_result.h"
#include "scada/data_value.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"
#include <chrono>

#include <boost/json.hpp>

#include <algorithm>
#include <cmath>
#include <random>

namespace scada {

namespace {

DataValue MakeValueAt(Variant value, scada::Time time) {
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

void LocalHistoryService::SetNowOverride(scada::Time now) {
  now_override_ = now;
}

scada::Time LocalHistoryService::Now() const {
  return scada::IsNull(now_override_) ? scada::Now() : now_override_;
}

void LocalHistoryService::LoadFromJson(const boost::json::value& root,
                                       const NodeExistsPredicate& node_exists) {
  // Optional frozen clock, so regenerating screenshots doesn't shift every
  // rendered timestamp.
  if (const auto* jnow = root.as_object().if_contains("now")) {
    if (auto now = base::TimeFromString(std::string(jnow->as_string()),
                                        /*is_local=*/true))
      SetNowOverride(*now);
  }

  // Raw-history base values from the `nodes` array, plus an optional per-node
  // `history_stddev` that overrides the default noise amplitude.
  for (const auto& jn : root.at("nodes").as_array()) {
    if (auto* bv = jn.as_object().if_contains("base_value")) {
      const NodeId node_id =
          NodeIdFromScadaString(std::string_view(jn.at("id").as_string()));

      // A node the caller could not create has no history either; see the
      // NodeExistsPredicate note on LoadFromJson.
      if (node_exists && !node_exists(node_id))
        continue;

      std::optional<double> noise_stddev;
      if (auto* sd = jn.as_object().if_contains("history_stddev"))
        noise_stddev = sd->to_number<double>();
      SetRawProfile(node_id, bv->to_number<double>(), noise_stddev);
    }
  }

  // Events from the `events` array.
  const auto now = Now();
  for (const auto& je : root.at("events").as_array()) {
    Event e;
    e.event_id = static_cast<EventId>(je.at("id").as_int64());
    double hours_ago = je.at("hours_ago").to_number<double>();
    e.time = now - std::chrono::round<std::chrono::microseconds>(
                       std::chrono::duration<double>{hours_ago * 3600});
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

CoStatusOr<HistoryReadRawResult> LocalHistoryService::HistoryReadRaw(
    HistoryReadRawDetails details) {
  co_return ReadRaw(std::move(details));
}

CoStatusOr<HistoryReadEventsResult> LocalHistoryService::HistoryReadEvents(
    NodeId node_id,
    scada::Time from,
    scada::Time to,
    EventFilter filter) {
  co_return ReadEvents(std::move(node_id), from, to, std::move(filter));
}

HistoryReadRawResult LocalHistoryService::ReadRaw(
    HistoryReadRawDetails details) const {
  // Anchor the synthesized series to the requested window when it has a
  // finite end: consumers (TimedDataFetcher) filter returned values by the
  // requested range, so a series pinned to a frozen "now" would otherwise
  // vanish for wall-clock-ranged queries, and vice versa.
  // A history read whose upper bound is null or the "current-only" sentinel
  // (Time::Max — used by live consumers that want the latest sample, not a
  // finite window) has no real end time. Anchoring the synthesized series to
  // Max would spread its 48 points across geological time, so every point but
  // the first falls outside any real query window and the series reads flat.
  // Treat an unbounded end as "now".
  const auto now = (scada::IsNull(details.to) || details.to == scada::kMaxTime)
                       ? Now()
                       : details.to;

  // Span the requested window when it is finite so every consumer gets a
  // fully-populated series regardless of its range — a graph's 24 h span and
  // a table row's 1 h sparkline window both read 48 points (a 24 h request
  // keeps the historical 30-minute spacing exactly). An open-ended request
  // falls back to that 30-minute spacing.
  //
  // The spacing is capped at that same 30 minutes, so a window far wider than
  // the data is meant to cover cannot stretch the series past a day. A
  // consumer may legitimately ask for "everything up to now" — the trend
  // probes for its earliest available sample to fill the left edge of the
  // plot — and spreading 48 points from the epoch to now put them ~14 months
  // apart, leaving every point but the last outside any real view. The graph
  // then drew nothing, with correct axes and a populated legend, so the empty
  // plot looked like a rendering quirk rather than bad data. Capping keeps
  // such a request answerable with the same 24 h series a bounded one gets.
  scada::Duration interval = std::chrono::minutes{30};
  if (!scada::IsNull(details.from) && details.from < now)
    interval = std::min((now - details.from) / 48, interval);

  // No profile, no history. The service is backed by an explicit table of
  // per-node base values; a node absent from it is one the caller never
  // described, and synthesizing a series for it anyway (this used to fall back
  // to a mean of 100.0) hands the consumer a convincing trend for a node that
  // may not even exist — a table row would paint a sparkline beside its "no
  // data" quality mark. An empty result says what is true: nothing is stored
  // for this node.
  auto profile = raw_profiles_.find(details.node_id);
  if (profile == raw_profiles_.end())
    return HistoryReadRawResult{};

  const double base_value = profile->second.base_value;
  const std::optional<double> noise_stddev = profile->second.noise_stddev;

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
      .values = std::move(values),
  };
}

HistoryReadEventsResult LocalHistoryService::ReadEvents(
    NodeId node_id,
    scada::Time /*from*/,
    scada::Time /*to*/,
    EventFilter /*filter*/) const {
  // Scoped by source node. A read rooted at the Server object (or at no node
  // at all) is the whole-server journal and gets everything; a read on any
  // other node — a device log asking about its own device — gets only that
  // node's events.
  //
  // Without the distinction every reader sees every seeded event, so two views
  // fed from one fixture cannot be told apart: a device log would list another
  // device's traffic, which is a convincing enough lie to ship in a
  // screenshot. The time range and filter are still ignored; captures seed
  // exactly what they want shown.
  // A null node is "no particular node" — an unscoped read, like Server.
  if (node_id.is_null() || node_id == scada::id::Server) {
    return HistoryReadEventsResult{.events = events_};
  }

  HistoryReadEventsResult result;
  for (const Event& event : events_) {
    if (event.source_node_id == node_id)
      result.events.push_back(event);
  }
  return result;
}

}  // namespace scada
