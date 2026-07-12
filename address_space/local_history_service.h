#pragma once

#include "scada/event.h"
#include "scada/history_service.h"
#include "scada/node_id.h"

#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace boost::json {
class value;
}

namespace scada {

// In-memory HistoryService backed by a static table of per-node base values
// (synthesized into a 48-point raw-history series on demand) and a fixed list
// of events.
//
// Intended for tests, demos, and screenshot tooling that need a SCADA back-end
// driven by static data rather than a real server.
class LocalHistoryService : public HistoryService {
 public:
  LocalHistoryService();
  ~LocalHistoryService() override;

  // Raw reads for `node_id` will return a 48-point normal distribution around
  // `base_value` at 30-minute intervals ending at "now" (see SetNowOverride).
  // The standard deviation defaults to 5% of |base_value|; pass `noise_stddev`
  // to override it with an absolute value. An explicit override lets a node
  // with a narrow engineering-unit range (e.g. grid frequency at 50 Hz within
  // [49.95, 50.05]) synthesize a series that stays inside that band instead of
  // overshooting it.
  void SetRawProfile(const NodeId& node_id,
                     double base_value,
                     std::optional<double> noise_stddev = std::nullopt);

  void AddEvent(Event event);

  // Freezes the service's notion of "now". Event timestamps and the end of
  // synthesized raw series anchor to `now` instead of the wall clock, which
  // keeps rendered timestamps stable across runs (screenshot generation
  // depends on this). Call before LoadFromJson; a null Time restores the
  // wall clock.
  void SetNowOverride(base::Time now);

  // Populates raw profiles from `nodes` and events from `events` of a
  // screenshot-style JSON document. Events are timestamped at load time as
  // `now - hours_ago * 1h`. An optional top-level `now` key ("YYYY-MM-DD
  // HH:MM:SS", local time) applies SetNowOverride before timestamping. A node
  // may carry an optional `history_stddev` (absolute standard deviation of the
  // synthesized raw series); it overrides the default 5%-of-|base_value| noise.
  void LoadFromJson(const boost::json::value& root);

  // HistoryService
  Awaitable<HistoryReadRawResult> HistoryReadRaw(
      HistoryReadRawDetails details) override;
  Awaitable<HistoryReadEventsResult> HistoryReadEvents(
      NodeId node_id,
      base::Time from,
      base::Time to,
      EventFilter filter) override;

 private:
  static UInt32 ParseSeverity(std::string_view s);
  base::Time Now() const;
  HistoryReadRawResult ReadRaw(HistoryReadRawDetails details) const;
  HistoryReadEventsResult ReadEvents(NodeId node_id,
                                     base::Time from,
                                     base::Time to,
                                     EventFilter filter) const;

  // Synthesized raw-history profile for a node: the series mean and, optionally,
  // an explicit absolute standard deviation (falls back to 5% of |base_value|).
  struct RawProfile {
    double base_value = 0.0;
    std::optional<double> noise_stddev;
  };

  std::unordered_map<NodeId, RawProfile> raw_profiles_;
  std::vector<Event> events_;
  base::Time now_override_;
};

}  // namespace scada
