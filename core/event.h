#pragma once

#include "base/strings/string16.h"
#include "core/configuration_types.h"
#include "core/data_value.h"

namespace scada {

// sys event severities
enum EventSeverity {
  kSeverityMin =	0, // silent
  kSeverityVerbose = 20, // verbose
  kSeverityNormal = 50, // normal
  kSeverityWarning = 60, // warning
  kSeverityCritical = 80, // critical
  kSeverityMax = 100 // max
};

using EventAcknowledgeId = unsigned;

class Event {
 public:
  enum ChangeFlags {
    EVT_VAL    = 0x0001, // value changed
    EVT_QUAL   = 0x0002, // quality changed
    EVT_LIM    = 0x0004, // limit changed
    EVT_USER   = 0x0008, // user event
    EVT_SUBS   = 0x0010, // subsystem event
    EVT_MAN    = 0x0020, // manual input
    EVT_LOCK   = 0x0040, // locked
    EVT_CTRL   = 0x0080, // control
    EVT_BACKUP = 0x0100, // locked
  };

  enum EventType { ACKED = 1,
                   UNACKED = 2 };

  Event()
      : change_mask(0),
        severity(kSeverityNormal),
        acked(false),
        acknowledge_id(0) {
  }

  NodeId     type_id;
  base::Time time;
  unsigned   change_mask;
  unsigned   severity;
  NodeId     node_id;
  NodeId     user_id;
  Variant    value;
  Qualifier  qualifier;
  base::string16 message;
  bool       acked;
  EventAcknowledgeId acknowledge_id;
  DateTime   acknowledged_time;
  NodeId     acknowledged_user_id;
};

} // namespace scada
