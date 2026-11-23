#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/check.h"
#include "scada/history_service.h"
// Releases `continuation_point` on `service`, as `context`'s caller: a server
// judges the release like the read that made the point (ReadHistory).
inline void CancelHistory(AnyExecutor executor,
                          scada::HistoryService& service,
                          scada::ServiceContext context,
                          const scada::HistoryReadRawDetails& details,
                          scada::ByteString&& continuation_point) {
  scada::base::Check(!continuation_point.empty());

  auto cancel_details = details;
  cancel_details.release_continuation_point = true;
  cancel_details.continuation_point = std::move(continuation_point);
  CoSpawn(std::move(executor),
          [&service, context = std::move(context),
           cancel_details =
               std::move(cancel_details)]() mutable -> Awaitable<void> {
            [[maybe_unused]] auto result = co_await service.HistoryReadRaw(
                std::move(context), std::move(cancel_details));
          });
}

// ScopedContinuationPoint

// Owns a history continuation point and releases it, as the caller that read
// it, unless `release()` hands it on.
class ScopedContinuationPoint {
 public:
  ScopedContinuationPoint() {}

  ScopedContinuationPoint(AnyExecutor executor,
                          scada::HistoryService& service,
                          scada::ServiceContext context,
                          scada::HistoryReadRawDetails details,
                          scada::ByteString continuation_point)
      : executor_{std::move(executor)},
        service_{&service},
        context_{std::move(context)},
        details_{std::move(details)},
        continuation_point_{std::move(continuation_point)} {
    if (continuation_point_.empty()) {
      details_ = {};
      service_ = nullptr;
    }
  }

  ~ScopedContinuationPoint() { reset(); }

  ScopedContinuationPoint(ScopedContinuationPoint&& source)
      : executor_{std::move(source.executor_)},
        service_{source.service_},
        context_{std::move(source.context_)},
        details_{std::move(source.details_)},
        continuation_point_{std::move(source.continuation_point_)} {
    source.service_ = nullptr;
  }

  ScopedContinuationPoint& operator=(ScopedContinuationPoint&& source) {
    if (&source != this) {
      reset();
      executor_ = std::move(source.executor_);
      service_ = source.service_;
      context_ = std::move(source.context_);
      details_ = std::move(source.details_);
      continuation_point_ = std::move(source.continuation_point_);
      source.service_ = nullptr;
    }
    return *this;
  }

  bool empty() const { return !service_; }

  void reset() {
    if (service_) {
      CancelHistory(executor_, *service_, context_, details_,
                    std::move(continuation_point_));
      continuation_point_.clear();
      service_ = nullptr;
    }
  }

  scada::ByteString release() {
    service_ = nullptr;
    return std::move(continuation_point_);
  }

 private:
  AnyExecutor executor_;
  scada::HistoryService* service_ = nullptr;
  scada::ServiceContext context_;
  scada::HistoryReadRawDetails details_;
  scada::ByteString continuation_point_;
};
