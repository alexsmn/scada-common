#pragma once

#include "base/any_executor.h"
#include "base/boost_log.h"
#include "common/history_util.h"
#include "node_service/node_ref.h"
#include "scada/date_time_range.h"
#include "timed_data/timed_data_buffer_fwd.h"

struct TimedDataFetcherContext {
  TimedDataBuffer& buffer_;
  AnyExecutor executor_;
  scada::HistoryService& history_service_;
  const scada::AggregateFilter aggregate_filter_;
};

class TimedDataFetcher : private TimedDataFetcherContext,
                         public std::enable_shared_from_this<TimedDataFetcher> {
 public:
  explicit TimedDataFetcher(TimedDataFetcherContext&& context);

  void SetNode(NodeRef node) { node_ = node; }

  void FetchNextGap();

 private:
  void FetchMore(ScopedContinuationPoint continuation_point);

  void OnHistoryReadRawComplete(std::vector<scada::DataValue> values,
                                ScopedContinuationPoint continuation_point);

  NodeRef node_;

  bool querying_ = false;
  scada::DateTimeRange querying_range_;

  inline static BoostLogger logger_{LOG_NAME("TimedDataFetcher")};
};
