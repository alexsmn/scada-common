#pragma once

#include "scada/history_service.h"

inline void CancelHistory(scada::HistoryService& service,
                          const scada::HistoryReadRawDetails& details,
                          scada::ByteString&& continuation_point) {
  assert(!continuation_point.empty());

  auto cancel_details = details;
  cancel_details.release_continuation_point = true;
  cancel_details.continuation_point = std::move(continuation_point);
  service.HistoryReadRaw(cancel_details,
                         [](scada::HistoryReadRawResult&& result) {});
}

// ScopedContinuationPoint

class ScopedContinuationPoint {
 public:
  ScopedContinuationPoint() {}

  ScopedContinuationPoint(scada::HistoryService& service,
                          scada::HistoryReadRawDetails details,
                          scada::ByteString continuation_point)
      : service_{&service},
        details_{std::move(details)},
        continuation_point_{std::move(continuation_point)} {
    if (continuation_point_.empty()) {
      details_ = {};
      service_ = nullptr;
    }
  }

  ~ScopedContinuationPoint() { reset(); }

  ScopedContinuationPoint(ScopedContinuationPoint&& source)
      : service_{source.service_},
        details_{std::move(source.details_)},
        continuation_point_{source.continuation_point_} {
    source.service_ = nullptr;
  }

  ScopedContinuationPoint& operator=(ScopedContinuationPoint&& source) {
    if (&source != this) {
      service_ = source.service_;
      details_ = std::move(source.details_);
      continuation_point_ = std::move(source.continuation_point_);
      source.service_ = nullptr;
    }
    return *this;
  }

  bool empty() const { return !service_; }

  void reset() {
    if (service_) {
      CancelHistory(*service_, details_, std::move(continuation_point_));
      continuation_point_.clear();
      service_ = nullptr;
    }
  }

  scada::ByteString release() {
    service_ = nullptr;
    return std::move(continuation_point_);
  }

 private:
  scada::HistoryService* service_ = nullptr;
  scada::HistoryReadRawDetails details_;
  scada::ByteString continuation_point_;
};
