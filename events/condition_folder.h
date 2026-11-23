#pragma once

#include "scada/event.h"

#include <map>
#include <optional>
#include <set>
#include <vector>

// Folds a Condition's notifications into the one event a current-alarm display
// holds for it (backlog 1117).
//
// Every state change of a Condition arrives as a new notification with a new
// EventId and the same ConditionId (OPC UA Part 9 §5.5.2 ConditionType,
// https://reference.opcfoundation.org/Core/Part9/v105/docs/5.5.2). The display
// wants one row per Condition, in its current state, so the folder keeps one
// entry per ConditionId and answers with that entry's current event:
//
//  - its fields are the latest notification's (time, message, ActiveState);
//  - its `event_id` is the local id the entry was given when it appeared (the
//    first notification's), so a row keeps its identity across state changes;
//  - its `acked` is true only once EVERY notification the entry has seen is
//    acknowledged. A return to normal does not acknowledge the activation
//    before it, and ISA-18.2's "unacknowledged, returned to normal" state is
//    exactly that: the alarm cleared before anyone read it.
//
// The entry is dropped when its current event stops being retained
// (`scada::IsRetained`: acknowledged and returned to normal), so the
// Condition's next activation starts a new row. A plain event (no ConditionId)
// passes through unchanged.
//
// Qt-free and service-free so the state machine is unit-tested on its own.
class ConditionFolder {
 public:
  // What an acknowledged notification acknowledges.
  enum class AckScope {
    // Only itself: our server acknowledges notifications by EventId, and the
    // Client acknowledges every one a row still holds
    // (`UnackedNotifications`).
    kNotification,
    // The whole Condition: a third-party server's AckedState belongs to the
    // Condition, so its acknowledged notification settles every state before
    // it (OPC UA Part 9 §5.7.2 AcknowledgeableConditionType,
    // https://reference.opcfoundation.org/Core/Part9/v105/docs/5.7.2).
    kCondition,
  };

  explicit ConditionFolder(AckScope ack_scope = AckScope::kNotification)
      : ack_scope_{ack_scope} {}

  // Folds `notification` in and returns the current event for its row.
  scada::Event Fold(const scada::Event& notification);

  // The wire EventIds acknowledging the row `local_id` must acknowledge: every
  // notification of its Condition still unacknowledged. A plain event's own id.
  std::vector<scada::EventId> UnackedNotifications(
      scada::EventId local_id) const;

  // Marks every notification of the row `local_id` acknowledged without a
  // server round trip — a third-party server confirms an Acknowledge by its
  // result, not by a notification. Returns the row's new current event, or
  // nullopt when `local_id` is not a Condition row.
  std::optional<scada::Event> AcknowledgeLocally(scada::EventId local_id,
                                                 scada::Time ack_time);

  void Clear();

 private:
  struct Entry {
    scada::EventId local_id = 0;
    scada::Event latest;
    std::set<scada::EventId> unacked;
    // The last acknowledgement seen, for the current event once every
    // notification is acknowledged (`Event::is_valid` wants a time).
    scada::Time ack_time = scada::kNullTime;
    scada::NodeId ack_user_id;
  };

  static bool IsNewer(const scada::Event& candidate,
                      const scada::Event& latest);
  static scada::Event CurrentEvent(const Entry& entry);

  // Drops `condition_id`'s entry when `current` is no longer retained.
  void ForgetIfDone(const scada::NodeId& condition_id,
                    const scada::Event& current);

  const AckScope ack_scope_;
  std::map<scada::NodeId, Entry> entries_;
  std::map<scada::EventId, scada::NodeId> condition_by_local_id_;
};
