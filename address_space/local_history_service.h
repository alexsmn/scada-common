#pragma once

#include "scada/event.h"
#include "scada/history_service.h"
#include "scada/node_id.h"

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
  // `base_value` (std = 5% of |base_value|) at 30-minute intervals ending at
  // "now" (see SetNowOverride).
  void SetRawProfile(const NodeId& node_id, double base_value);

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
  // HH:MM:SS", local time) applies SetNowOverride before timestamping.
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

  std::unordered_map<NodeId, double> raw_profiles_;
  std::vector<Event> events_;
  base::Time now_override_;
};

}  // namespace scada
