#pragma once

#include "base/strings/string16.h"

namespace scada {

enum class StatusSeverity {
  // Indicates that the operation was successful and the associated results may
  // be used.
  Good = 0,

  // Indicates that the operation was partially successful and that associated
  // results might not be suitable for some purposes.
  Uncertain = 1,

  // Indicates that the operation failed and any associated results cannot be
  // used.
  Bad = 2,

  // Reserved for future use. All Clients should treat a StatusCode with this
  // severity as “Bad”.
  Reserved = 3
};

enum class StatusCode : unsigned {
  Good = static_cast<unsigned>(StatusSeverity::Good),
  Good_Pending = Good | 1,  // Async operation started
  Good_Sporadic = Good | 2,
  Good_Backup = Good | 3,     // Uncertain_SubNormal
  Good_Manual = Good | 4,     // Good_LocalOverride
  Good_Simulated = Good | 5,  // Good_LocalOverride
  Uncertain = static_cast<unsigned>(StatusSeverity::Uncertain) << 14,
  Uncertain_DeviceFlag = Uncertain | 1,
  Uncertain_Misconfigured = Uncertain | 2,
  Uncertain_Disconnected = Uncertain | 3,
  Uncertain_NotUpdated = Uncertain | 4,
  // Lock command was not changed state, because object is already
  // locked/unlocked.
  Uncertain_StateWasNotChanged = Uncertain | 5,
  Bad = static_cast<unsigned>(StatusSeverity::Bad) << 14,
  Bad_WrongLoginCredentials = Bad | 1,
  Bad_UserIsAlreadyLoggedOn = Bad | 2,
  Bad_UnsupportedProtocolVersion = Bad | 3,
  Bad_ObjectIsBusy = Bad | 4,
  Bad_WrongNodeId = Bad | 5,
  Bad_WrongDeviceId = Bad | 6,
  // Trying to perform command on disconnected object.
  Bad_Disconnected = Bad | 7,
  Bad_SessionForcedLogoff = Bad | 8,
  Bad_Timeout = Bad | 9,
  Bad_CantDeleteDependentNode = Bad | 10,
  Bad_ServerWasShutDown = Bad | 11,
  Bad_WrongMethodId = Bad | 12,
  Bad_CantDeleteOwnUser = Bad | 13,
  Bad_DuplicateNodeId = Bad | 14,
  Bad_UnsupportedFileVersion = Bad | 15,
  Bad_WrongTypeId = Bad | 16,
  Bad_WrongParentId = Bad | 17,
  Bad_SessionIsLoggedOff = Bad | 18,
  Bad_WrongSubscriptionId = Bad | 19,
  Bad_WrongIndex = Bad | 20,
  Bad_Iec60870UnknownType = Bad | 21,
  Bad_Iec60870UnknownCot = Bad | 22,
  Bad_Iec60870UnknownDevice = Bad | 23,
  Bad_Iec60870UnknownAddress = Bad | 24,
  Bad_Iec60870UnknownError = Bad | 25,
  Bad_WrongCallArguments = Bad | 26,
  Bad_CantParseString = Bad | 27,
  Bad_TooLongString = Bad | 28,
  Bad_WrongPropertyId = Bad | 29,
  Bad_WrongReferenceId = Bad | 30,
  Bad_WrongNodeClass = Bad | 31,
  Bad_WrongAttributeId = Bad | 32,
  Bad_Iec61850Error = Bad | 33,
};

enum class StatusLimit {
  // The value is free to change.
  None = 0,

  // The value is at the lower limit for the data source.
  Low = 1,

  // The value is at the higher limit for the data source.
  High = 2,

  // The value is constant and cannot change.
  Constant,
};

inline StatusSeverity GetSeverity(StatusCode code) noexcept {
  return static_cast<StatusSeverity>(static_cast<unsigned>(code) >> 14);
}
inline bool IsGood(StatusCode code) noexcept {
  return GetSeverity(code) == StatusSeverity::Good;
}

inline bool IsBad(StatusCode code) noexcept {
  return GetSeverity(code) == StatusSeverity::Bad;
}

class Status {
 public:
  constexpr Status(StatusCode code) noexcept
      : full_code_(static_cast<unsigned>(code) << 16) {}

  static Status FromFullCode(unsigned full_code);

  explicit operator bool() const noexcept { return !bad(); }
  bool operator!() const noexcept { return bad(); }

  StatusSeverity severity() const noexcept {
    return static_cast<StatusSeverity>(full_code_ >> 30);
  }

  bool good() const noexcept { return severity() == StatusSeverity::Good; }
  bool uncertain() const noexcept {
    return severity() == StatusSeverity::Uncertain;
  }
  bool bad() const noexcept { return severity() == StatusSeverity::Bad; }

  StatusLimit limit() const noexcept {
    return static_cast<StatusLimit>(full_code_ & 3);
  }

  void set_limit(StatusLimit limit) noexcept {
    full_code_ &= ~3U;
    full_code_ |= static_cast<unsigned>(limit);
  }

  StatusCode code() const noexcept {
    return static_cast<StatusCode>(full_code_ >> 16);
  }

  unsigned full_code() const noexcept { return full_code_; }

 private:
  unsigned full_code_;
};

}  // namespace scada

std::string ToString(scada::StatusCode status_code);
base::string16 ToString16(scada::StatusCode status_code);

std::string ToString(const scada::Status& status);
base::string16 ToString16(const scada::Status& status);
