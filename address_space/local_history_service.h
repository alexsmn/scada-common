#pragma once

#include "scada/co_result.h"
#include "scada/event.h"
#include "scada/history_service.h"
#include "scada/node_id.h"

#include <functional>
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
// The table is the whole truth: a raw read for a node with no registered
// profile returns no values rather than inventing a series, so a consumer can
// never render a trend for data the fixture never described.
//
// Intended for tests, demos, and screenshot tooling that need a SCADA back-end
// driven by static data rather than a real server.
class LocalHistoryService : public HistoryService {
 public:
  LocalHistoryService();
  ~LocalHistoryService() override;

  // Registers `node_id` as having history. Raw reads for it return a 48-point
  // normal distribution around `base_value` at 30-minute intervals ending at
  // "now" (see SetNowOverride).
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
  void SetNowOverride(scada::Time now);

  // Answers whether a node exists in the caller's address space. See
  // LoadFromJson.
  using NodeExistsPredicate = std::function<bool(const NodeId&)>;

  // Populates raw profiles from `nodes` and events from `events` of a
  // screenshot-style JSON document. Events are timestamped at load time as
  // `now - hours_ago * 1h`. An optional top-level `now` key ("YYYY-MM-DD
  // HH:MM:SS", local time) applies SetNowOverride before timestamping. A node
  // may carry an optional `history_stddev` (absolute standard deviation of the
  // synthesized raw series); it overrides the default 5%-of-|base_value| noise.
  //
  // Pass `node_exists` when the document's `nodes` array can name nodes the
  // caller never created (the screenshot fixture, for instance, can only build
  // a node that its `tree` gives a parent). Without it, such a node still gets
  // a synthesized series, so a consumer reads a plausible trend for a node
  // that does not exist — fabricated data of exactly the kind the "no data"
  // quality mark exists to prevent. Nodes the predicate rejects get no
  // profile, so a raw read for them comes back empty.
  void LoadFromJson(const boost::json::value& root,
                    const NodeExistsPredicate& node_exists = {});

  // HistoryService
  CoStatusOr<HistoryReadRawResult> HistoryReadRaw(
      HistoryReadRawDetails details) override;
  CoStatusOr<HistoryReadEventsResult> HistoryReadEvents(
      NodeId node_id,
      scada::Time from,
      scada::Time to,
      EventFilter filter) override;

 private:
  static UInt32 ParseSeverity(std::string_view s);
  scada::Time Now() const;
  HistoryReadRawResult ReadRaw(HistoryReadRawDetails details) const;
  HistoryReadEventsResult ReadEvents(NodeId node_id,
                                     scada::Time from,
                                     scada::Time to,
                                     EventFilter filter) const;

  // Synthesized raw-history profile for a node: the series mean and,
  // optionally, an explicit absolute standard deviation (falls back to 5% of
  // |base_value|).
  struct RawProfile {
    double base_value = 0.0;
    std::optional<double> noise_stddev;
  };

  std::unordered_map<NodeId, RawProfile> raw_profiles_;
  std::vector<Event> events_;
  scada::Time now_override_ = scada::kNullTime;
};

}  // namespace scada
