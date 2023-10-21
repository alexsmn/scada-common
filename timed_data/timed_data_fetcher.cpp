#include "timed_data/timed_data_impl.h"

#include "base/cancelation.h"
#include "base/debug_util.h"
#include "base/executor.h"
#include "base/format_time.h"
#include "base/interval.h"
#include "common/timed_data_util.h"
#include "model/node_id_util.h"
#include "scada/history_service.h"
#include "timed_data/timed_data_fetcher.h"
#include "timed_data/timed_data_util.h"

#include "base/debug_util-inl.h"

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

  auto gap = timed_data_view_.FindNextGap();
  if (!gap)
    return;

  assert(!IsEmptyInterval(*gap));

  querying_ = true;
  querying_range_ = *gap;

  LOG_INFO(logger_) << "Querying history"
                    << LOG_TAG("From", FormatTime(querying_range_.first))
                    << LOG_TAG("To", FormatTime(querying_range_.second));

  // Query history in the backward direction.
  scada::HistoryReadRawDetails details{node_.node_id(), querying_range_.first,
                                       querying_range_.second, kMaxReadCount,
                                       aggregate_filter_};
  // Canot use |BindCancelation| as |ScopedContinuationPoint| must always be
  // handled.
  history_service_.HistoryReadRaw(
      details,
      BindExecutor(executor_, [weak_ptr = weak_from_this(),
                               &history_service = history_service_,
                               details](scada::HistoryReadRawResult result) {
        ScopedContinuationPoint scoped_continuation_point{
            history_service, details, std::move(result.continuation_point)};
        if (auto ptr = weak_ptr.lock()) {
          ptr->OnHistoryReadRawComplete(std::move(result.values),
                                        std::move(scoped_continuation_point));
        }
      }));
}

void TimedDataFetcher::FetchMore(ScopedContinuationPoint continuation_point) {
  assert(querying_);
  assert(!continuation_point.empty());

  if (!node_) {
    LOG_INFO(logger_) << "Node was deleted";
    querying_ = false;
    return;
  }

  // Reset query if the requested range is no more interesting.
  auto gap = timed_data_view_.FindNextGap();
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
  // Cannot use |BindCancelation| as |ScopedContinuationPoint| must always be
  // handled.
  history_service_.HistoryReadRaw(
      details,
      BindExecutor(executor_, [weak_ptr = weak_from_this(),
                               &history_service = history_service_,
                               details](scada::HistoryReadRawResult result) {
        ScopedContinuationPoint scoped_continuation_point{
            history_service, details, std::move(result.continuation_point)};
        if (auto ptr = weak_ptr.lock()) {
          ptr->OnHistoryReadRawComplete(std::move(result.values),
                                        std::move(scoped_continuation_point));
        }
      }));
}

void TimedDataFetcher::OnHistoryReadRawComplete(
    std::vector<scada::DataValue> values,
    ScopedContinuationPoint continuation_point) {
  assert(querying_);
  assert(IsTimeSorted(values));
  assert(std::none_of(
      values.begin(), values.end(),
      [](const scada::DataValue& v) { return v.server_timestamp.is_null(); }));

  auto ref = shared_from_this();

  timed_data_view_.ReplaceRange(values);

  scada::DateTime ready_to;
  if (continuation_point.empty())
    ready_to = querying_range_.second;
  else if (!values.empty())
    ready_to = values.back().source_timestamp;

  if (!ready_to.is_null()) {
    LOG_INFO(logger_) << "Query result" << LOG_TAG("ValueCount", values.size())
                      << LOG_TAG("ReadFrom", FormatTime(querying_range_.first))
                      << LOG_TAG("ReadTo", FormatTime(ready_to));

    timed_data_view_.AddReadyRange({querying_range_.first, ready_to});

    assert(ready_to >= querying_range_.first);
    assert(ready_to <= querying_range_.second);
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
