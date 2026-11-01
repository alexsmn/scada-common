#include "address_space/local_history_service.h"
#include "model/namespaces.h"

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

UInt32 LocalHistoryService::ParseSeverity(const boost::json::value& value) {
  // A raw OPC UA severity, for a fixture that needs a value the three band
  // literals cannot name. Out-of-range numbers are clamped rather than
  // rejected: this is fixture data, and a severity of 0 or 4000 would
  // otherwise reach the severity banding as a value the 1..1000 scale has no
  // meaning for.
  if (value.is_number()) {
    const std::int64_t severity = value.to_number<std::int64_t>();
    return static_cast<UInt32>(std::clamp<std::int64_t>(severity, 1, 1000));
  }

  const std::string_view s{value.as_string()};
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
      // Only a number has a raw-history profile. A fixture value can also be a
      // boolean or a string — a link's t1 flag, a protocol's state — and
      // synthesizing a normal distribution around one is meaningless; before
      // this skip existed, `to_number` threw and took the whole fixture with
      // it.
      if (!bv->is_int64() && !bv->is_uint64() && !bv->is_double())
        continue;

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
    e.severity = ParseSeverity(je.at("severity"));
    e.message = LocalizedText{
        UtfConvert<char16_t>(std::string(je.at("message").as_string()))};
    e.source_node_id =
        NodeIdFromScadaString(std::string_view(je.at("node_id").as_string()));
    e.change_mask = static_cast<UInt32>(je.at("change_mask").as_int64());
    // Optional `event_type`: an NS0 numeric id. Without it an event stays the
    // default SystemEventType, which is what every process event here is. The
    // audit trail needs its own types, because a view scoped to the
    // AuditEventType subtree can only be exercised by events that carry one.
    if (const auto* jt = je.as_object().if_contains("event_type")) {
      e.event_type_id =
          NodeId{static_cast<NumericId>(jt->to_number<std::int64_t>()),
                 NamespaceIndexes::NS0};
    }
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

  // Every read is a slice of ONE stored series, never a series shaped to the
  // request. This used to synthesize 48 points spread across whatever window
  // was asked for, so two overlapping reads disagreed about the same instants
  // — and a client's timed-data cache merges reads per node, so a capture's
  // sparkline depended on which other windows had read that node before it
  // (backlog 949: `table.png` differed between a full pass and `--only`).
  //
  // The series ends at "now" and never past it, whatever `to` says: a
  // historian holds nothing from the future, and the old window-anchored form
  // happily returned samples up to a `to` an hour ahead. An unbounded `to`
  // (null, or the Time::Max "current-only" sentinel) is simply no upper bound.
  // It begins kRawSeriesSpan earlier, so a read reaching back to the epoch —
  // the trend probes for its earliest sample that way — still gets a day of
  // samples instead of points spread across the gap.
  const scada::Time now = Now();
  const scada::Time to =
      (scada::IsNull(details.to) || details.to > now) ? now : details.to;

  // Samples sit on a grid of absolute times (whole multiples of kRawSampleStep
  // since the epoch), so the set of instants does not move with "now" either:
  // a wall-clock-driven consumer reading twice gets the same values at the
  // instants both reads cover.
  const auto step = kRawSampleStep.count();
  const auto now_us = now.time_since_epoch().count();
  const std::int64_t last_index =
      now_us >= 0 ? now_us / step : -((-now_us + step - 1) / step);
  constexpr std::int64_t kSampleCount = kRawSeriesSpan / kRawSampleStep;

  std::vector<DataValue> values;
  for (std::int64_t index = last_index - kSampleCount + 1; index <= last_index;
       ++index) {
    const scada::Time time{scada::Duration{index * step}};
    if (time >= now)
      continue;  // History is what was stored BEFORE now.
    if (!scada::IsNull(details.from) && time < details.from)
      continue;
    if (time > to)
      break;
    values.push_back(MakeValueAt(
        Variant{SampleValue(details.node_id, profile->second, index)}, time));
  }

  return HistoryReadRawResult{
      .values = std::move(values),
  };
}

// static
double LocalHistoryService::SampleValue(const NodeId& node_id,
                                        const RawProfile& profile,
                                        std::int64_t sample_index) {
  // A sample is a pure function of (node, instant): linear between knots drawn
  // every kRawKnotStep, plus a small per-sample jitter. The knots carry the
  // shape a day-long trend shows; the jitter keeps an hour-long sparkline,
  // which spans only two or three knots, from reading as a ruled line.
  // Deterministic per node — seeded on the numeric id — so output is stable
  // across runs. The amplitude defaults to 5% of the mean, but a node may pin
  // an absolute standard deviation (e.g. so a narrow-range frequency series
  // stays inside its engineering-unit band).
  const std::uint32_t seed = node_id.is_numeric() ? node_id.numeric_id() : 0u;
  const double stddev =
      profile.noise_stddev.value_or(std::abs(profile.base_value) * 0.05);

  auto draw = [seed](std::int64_t index, std::uint32_t stream) {
    const auto u = static_cast<std::uint64_t>(index);
    std::seed_seq seq{seed, stream, static_cast<std::uint32_t>(u),
                      static_cast<std::uint32_t>(u >> 32)};
    std::mt19937 rng{seq};
    return std::normal_distribution<double>{0.0, 1.0}(rng);
  };

  constexpr std::int64_t kSamplesPerKnot = kRawKnotStep / kRawSampleStep;
  // Floor division, so a knot boundary does not depend on the sign of index.
  std::int64_t knot = sample_index / kSamplesPerKnot;
  if (sample_index % kSamplesPerKnot < 0)
    --knot;
  const double frac =
      static_cast<double>(sample_index - knot * kSamplesPerKnot) /
      kSamplesPerKnot;
  const double a = draw(knot, 0);
  const double b = draw(knot + 1, 0);
  const double shape = a + (b - a) * frac;
  const double jitter = draw(sample_index, 1) * kRawJitterRatio;
  return profile.base_value + stddev * (shape + jitter);
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
