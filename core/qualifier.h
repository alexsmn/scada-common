#pragma once

#include "core/status.h"

namespace scada {

class Qualifier {
 public:
  enum Limit {
    LIMIT_NORMAL = 0,
    LIMIT_LO     = 1,
    LIMIT_HI     = 2,
    LIMIT_LOLO   = 3,
    LIMIT_HIHI   = 4,
  };
 
  enum {
    MANUAL        = 0x0002,
    DELETED_LOCKED = 0x0004,
    BACKUP        = 0x0008,

    SIMULATED     = 0x0100,
    SPORADIC      = 0x0400,

    BAD           = 0x0001, // Data with invalid flag
    FAILED        = 0x0040, // Can't connect object on server
    MISCONFIGURED = 0x0200, // Configuration is incorrect or device is disabled
    OFFLINE       = 0x0020, // Device connection is broken
    STALE         = 0x0080, // Device value is too old
    GENERAL_BAD   = BAD | FAILED | MISCONFIGURED | OFFLINE | STALE,

    // Flags only for current time.
    REALTIME_FLAGS = FAILED | MISCONFIGURED | OFFLINE | STALE | BACKUP,

    LIMIT_MASK    = 0x7000, // 0x1000 | 0x2000 | 0x4000
    LIMIT_SHIFT   = 12,
  };

  Qualifier() : qualifier_(0) { }
  Qualifier(unsigned qualifier) : qualifier_(qualifier) { }

  void Update(unsigned remove, unsigned add) {
    qualifier_ = (qualifier_ & ~remove) | add;
  }

  void set_flag(unsigned flag, bool set) {
    if (set)
      qualifier_ |= flag;
    else
      qualifier_ &= ~flag;
  }
  
  void set_bad(bool set)       { set_flag(BAD, set); }
  void set_manual(bool set)    { set_flag(MANUAL, set); }
  void set_backup(bool set)    { set_flag(BACKUP, set); }
  void set_online(bool set)    { set_flag(OFFLINE, !set); }
  void set_failed(bool set)    { set_flag(FAILED, set); }
  void set_stale(bool set)     { set_flag(STALE, set); }
  void set_simulated(bool set) { set_flag(SIMULATED, set); }
  void set_misconfigured(bool set) { set_flag(MISCONFIGURED, set); }
  void set_sporadic(bool set)  { set_flag(SPORADIC, set); }
  
  void set_limit(Limit limit) {
    qualifier_ = (qualifier_ & ~LIMIT_MASK) |
                 ((limit << LIMIT_SHIFT) & LIMIT_MASK);
  }
  
  bool flag(unsigned flag) const { return (qualifier_ & flag) != 0; }

  // Return raised bad flags.
  unsigned general_bad() const { return qualifier_ & GENERAL_BAD; }
  
  bool bad() const         { return flag(BAD); }
  bool good() const        { return !flag(BAD); }
  bool manual() const			 { return flag(MANUAL); }
  bool backup() const			 { return flag(BACKUP); }
  bool offline() const		 { return flag(OFFLINE); }
  bool online() const      { return !flag(OFFLINE); }
  bool failed() const			 { return flag(FAILED); }
  bool stale() const       { return flag(STALE); }
  bool simulated() const   { return flag(SIMULATED); }
  bool misconfigured() const { return flag(MISCONFIGURED); }
  bool sporadic() const    { return flag(SPORADIC); }

  Limit limit() const { return static_cast<Limit>((qualifier_ & LIMIT_MASK) >> LIMIT_SHIFT); }
  
  unsigned raw() const { return qualifier_; }
  
  bool operator==(Qualifier other) const { return qualifier_ == other.qualifier_; }
  bool operator!=(Qualifier other) const { return qualifier_ != other.qualifier_; }
  
  Qualifier operator|(Qualifier other) const { return Qualifier(qualifier_ | other.qualifier_); }
  Qualifier& operator|=(Qualifier other) { qualifier_ |= other.qualifier_; return *this; }

  Status ToStatus() const;

 private:
  unsigned qualifier_;
};

inline Status Qualifier::ToStatus() const {
  Status status(StatusCode::Good);

  if (bad())
    status = Status(StatusCode::Uncertain_DeviceFlag);
  else if (offline())
    status = Status(StatusCode::Uncertain_Disconnected);
  else if (stale())
    status = Status(StatusCode::Uncertain_NotUpdated);
  else if (misconfigured())
    status = Status(StatusCode::Uncertain_Misconfigured);
  else if (manual())
    status = Status(StatusCode::Good_Manual);
  else if (backup())
    status = Status(StatusCode::Good_Backup);
  else if (simulated())
    status = Status(StatusCode::Good_Simulated);
  else if (sporadic())
    status = Status(StatusCode::Good_Sporadic);

  if (limit() == LIMIT_LOLO)
    status.set_limit(StatusLimit::Low);
  else if (limit() == LIMIT_HIHI)
    status.set_limit(StatusLimit::High);

  return status;
}

} // namespace scada