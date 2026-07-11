#include "timed_data/timed_data_impl.h"

#include "base/any_executor_dispatch.h"
#include "base/awaitable.h"
#include "base/cancelation.h"
#include "base/check.h"
#include "base/debug_util.h"
#include "base/format_time.h"
#include "base/interval.h"
#include "common/data_value_traits.h"
#include "common/timed_data_util.h"
#include "model/node_id_util.h"
#include "scada/history_service.h"
#include "timed_data/timed_data_fetcher.h"
#include "timed_data/timed_data_util.h"

namespace {
const size_t kMaxReadCount = 10000;
}

TimedDataFetcher::TimedDataFetcher(TimedDataFetcherContext&& context)
    : TimedDataFetcherContext{std::move(context)} {}

void TimedDataFetcher::FetchNextGap() {
  if (querying_)
    return;

  if (!node_)
    return;

  auto gap = buffer_.FindNextGap();
  if (!gap)
    return;

  base::Check(!IsEmptyInterval(*gap));

  querying_ = true;
  querying_range_ = *gap;

  LOG_INFO(logger_) << "Querying history"
                    << LOG_TAG("From", FormatTime(querying_range_.first))
                    << LOG_TAG("To", FormatTime(querying_range_.second));

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(), querying_range_.first,
                                       querying_range_.second, kMaxReadCount,
                                       aggregate_filter_};
  CoSpawn(executor_, weak_from_this(),
          [details](std::shared_ptr<TimedDataFetcher> self) -> Awaitable<void> {
            auto result =
                co_await self->history_service_.HistoryReadRaw(details);
            ScopedContinuationPoint scoped_continuation_point{
                self->executor_, self->history_service_, details,
                std::move(result.continuation_point)};
            self->OnHistoryReadRawComplete(
                std::move(result.values), std::move(scoped_continuation_point));
          });
}

void TimedDataFetcher::FetchMore(ScopedContinuationPoint continuation_point) {
  base::Check(querying_);
  base::Check(!continuation_point.empty());

  if (!node_) {
    LOG_INFO(logger_) << "Node was deleted";
    querying_ = false;
    return;
  }

  // Reset query if the requested range is no more interesting.
  auto gap = buffer_.FindNextGap();
  if (!gap || !IntervalContains(*gap, querying_range_)) {
    LOG_INFO(logger_) << "Query canceled" << LOG_TAG("Gap", ToString(*gap))
                      << LOG_TAG("Range", ToString(querying_range_));
    continuation_point.reset();
    querying_ = false;
    FetchNextGap();
    return;
  }

  LOG_INFO(logger_) << "Continue querying history"
                    << LOG_TAG("Range", ToString(querying_range_));

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(),
                                       querying_range_.second,
                                       querying_range_.first,
                                       kMaxReadCount,
                                       aggregate_filter_,
                                       false,
                                       continuation_point.release()};
  CoSpawn(executor_, weak_from_this(),
          [details](std::shared_ptr<TimedDataFetcher> self) -> Awaitable<void> {
            auto result =
                co_await self->history_service_.HistoryReadRaw(details);
            ScopedContinuationPoint scoped_continuation_point{
                self->executor_, self->history_service_, details,
                std::move(result.continuation_point)};
            self->OnHistoryReadRawComplete(
                std::move(result.values), std::move(scoped_continuation_point));
          });
}

void TimedDataFetcher::OnHistoryReadRawComplete(
    std::vector<scada::DataValue> values,
    ScopedContinuationPoint continuation_point) {
  base::Check(querying_);

  // History results come from a (possibly remote) history service; sanitize
  // malformed responses instead of panicking.
  const size_t dropped = std::erase_if(values, [](const scada::DataValue& v) {
    return v.server_timestamp.is_null();
  });
  if (dropped != 0) {
    LOG_WARNING(logger_) << "History read returned values without a server "
                            "timestamp"
                         << LOG_TAG("Count", dropped);
  }
  if (!IsTimeSorted(values)) {
    LOG_WARNING(logger_) << "History read returned unsorted values";
    std::ranges::stable_sort(values, std::less{},
                             &TimedDataTraits<scada::DataValue>::timestamp);
    // Drop duplicate timestamps to restore the strict ordering.
    auto duplicates = std::ranges::unique(
        values, std::equal_to{}, &TimedDataTraits<scada::DataValue>::timestamp);
    values.erase(duplicates.begin(), duplicates.end());
  }

  auto ref = shared_from_this();

  buffer_.ReplaceRange(values);

  scada::DateTime ready_to;
  if (continuation_point.empty()) {
    ready_to = querying_range_.second;
  } else if (!values.empty()) {
    // |ready_to| derives from server-provided timestamps; clamp it to the
    // queried range instead of trusting the response.
    ready_to = std::clamp(values.back().source_timestamp, querying_range_.first,
                          querying_range_.second);
  }

  if (!ready_to.is_null()) {
    LOG_INFO(logger_) << "Query result" << LOG_TAG("ValueCount", values.size())
                      << LOG_TAG("ReadFrom", FormatTime(querying_range_.first))
                      << LOG_TAG("ReadTo", FormatTime(ready_to));

    buffer_.AddReadyRange({querying_range_.first, ready_to});

    querying_range_.first = ready_to;
  }

  // Query next fragment of data if |from_| changed from moment of last query.
  // Note that |from_| is allowed to be increased.
  if (continuation_point.empty()) {
    LOG_INFO(logger_) << "Query completed";
    querying_ = false;
    FetchNextGap();

  } else {
    FetchMore(std::move(continuation_point));
  }
}
