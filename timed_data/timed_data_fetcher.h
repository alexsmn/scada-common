#pragma once

#include "common/history_util.h"

struct TimedDataFetcherContext {
  TimedDataView& timed_data_view_;
  const std::shared_ptr<Executor> executor_;
  scada::HistoryService& history_service_;
  const scada::AggregateFilter aggregate_filter_;
};

class TimedDataFetcher : private TimedDataFetcherContext,
                         public std::enable_shared_from_this<TimedDataFetcher> {
 public:
  explicit TimedDataFetcher(TimedDataFetcherContext&& context);

  void SetNode(NodeRef node) {}

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
