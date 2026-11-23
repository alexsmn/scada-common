#include "events/condition_folder.h"

scada::Event ConditionFolder::Fold(const scada::Event& notification) {
  if (notification.condition_id.is_null())
    return notification;

  auto [i, inserted] = entries_.try_emplace(notification.condition_id);
  Entry& entry = i->second;
  if (inserted) {
    entry.local_id = notification.event_id;
    entry.latest = notification;
    condition_by_local_id_[entry.local_id] = notification.condition_id;
  } else if (IsNewer(notification, entry.latest)) {
    entry.latest = notification;
  }

  if (notification.acked) {
    if (ack_scope_ == AckScope::kCondition)
      entry.unacked.clear();
    else
      entry.unacked.erase(notification.event_id);
    if (!scada::IsNull(notification.acknowledged_time)) {
      entry.ack_time = notification.acknowledged_time;
      entry.ack_user_id = notification.acknowledged_user_id;
    }
  } else {
    entry.unacked.insert(notification.event_id);
  }

  scada::Event current = CurrentEvent(entry);
  ForgetIfDone(notification.condition_id, current);
  return current;
}

std::vector<scada::EventId> ConditionFolder::UnackedNotifications(
    scada::EventId local_id) const {
  auto c = condition_by_local_id_.find(local_id);
  if (c == condition_by_local_id_.end())
    return {local_id};
  const Entry& entry = entries_.at(c->second);
  return {entry.unacked.begin(), entry.unacked.end()};
}

std::optional<scada::Event> ConditionFolder::AcknowledgeLocally(
    scada::EventId local_id,
    scada::Time ack_time) {
  auto c = condition_by_local_id_.find(local_id);
  if (c == condition_by_local_id_.end())
    return std::nullopt;
  const scada::NodeId condition_id = c->second;
  Entry& entry = entries_.at(condition_id);
  entry.unacked.clear();
  entry.ack_time = ack_time;
  scada::Event current = CurrentEvent(entry);
  ForgetIfDone(condition_id, current);
  return current;
}

void ConditionFolder::Clear() {
  entries_.clear();
  condition_by_local_id_.clear();
}

// static
bool ConditionFolder::IsNewer(const scada::Event& candidate,
                              const scada::Event& latest) {
  // The same notification again — typically its acknowledged copy — updates
  // in place; an acknowledged copy of an OLDER notification must not wind the
  // row back to a state the Condition has left.
  if (candidate.event_id == latest.event_id)
    return true;
  if (candidate.time != latest.time)
    return candidate.time > latest.time;
  return candidate.event_id > latest.event_id;
}

// static
scada::Event ConditionFolder::CurrentEvent(const Entry& entry) {
  scada::Event current = entry.latest;
  current.event_id = entry.local_id;
  current.acked = entry.unacked.empty();
  if (current.acked) {
    current.acknowledged_time =
        scada::IsNull(entry.ack_time) ? entry.latest.time : entry.ack_time;
    current.acknowledged_user_id = entry.ack_user_id;
  } else {
    current.acknowledged_time = scada::kNullTime;
    current.acknowledged_user_id = {};
  }
  return current;
}

void ConditionFolder::ForgetIfDone(const scada::NodeId& condition_id,
                                   const scada::Event& current) {
  if (scada::IsRetained(current))
    return;
  auto i = entries_.find(condition_id);
  if (i == entries_.end())
    return;
  condition_by_local_id_.erase(i->second.local_id);
  entries_.erase(i);
}
