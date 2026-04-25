#pragma once

#include "scada/coroutine_services.h"
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
class LocalHistoryService : public HistoryService,
                            public CoroutineHistoryService {
 public:
  LocalHistoryService();
  ~LocalHistoryService() override;

  // Raw reads for `node_id` will return a 48-point normal distribution around
  // `base_value` (std = 5% of |base_value|) at 30-minute intervals ending at
  // the time of the request.
  void SetRawProfile(const NodeId& node_id, double base_value);

  void AddEvent(Event event);

  // Populates raw profiles from `nodes` and events from `events` of a
  // screenshot-style JSON document. Events are timestamped at load time as
  // `base::Time::Now() - hours_ago * 1h`.
  void LoadFromJson(const boost::json::value& root);

  // HistoryService
  void HistoryReadRaw(const HistoryReadRawDetails& details,
                      const HistoryReadRawCallback& callback) override;
  void HistoryReadEvents(const NodeId& node_id,
                         base::Time from,
                         base::Time to,
                         const EventFilter& filter,
                         const HistoryReadEventsCallback& callback) override;

  // CoroutineHistoryService
  Awaitable<HistoryReadRawResult> HistoryReadRaw(
      HistoryReadRawDetails details) override;
  Awaitable<HistoryReadEventsResult> HistoryReadEvents(NodeId node_id,
                                                       base::Time from,
                                                       base::Time to,
                                                       EventFilter filter)
      override;

 private:
  static UInt32 ParseSeverity(std::string_view s);
  HistoryReadRawResult ReadRaw(HistoryReadRawDetails details) const;
  HistoryReadEventsResult ReadEvents(NodeId node_id,
                                     base::Time from,
                                     base::Time to,
                                     EventFilter filter) const;

  std::unordered_map<NodeId, double> raw_profiles_;
  std::vector<Event> events_;
};

}  // namespace scada
