#pragma once

#include "scada/history_service.h"
#include "scada/status_promise.h"

namespace scada {

// Returns either good `HistoryReadRawResult.status_code` or rejected status
// promise.
inline promise<HistoryReadRawResult> HistoryReadRawChunk(
    HistoryService& history_service,
    const HistoryReadRawDetails& details) {
  promise<HistoryReadRawResult> promise;
  // Cannot use `MakeStatusPromiseCallback` because it requires
  // `HistoryReadRawResult` to have `status` field.
  history_service.HistoryReadRaw(details, MakeStatusPromiseCallback(promise));
  return promise;
}

inline promise<std::vector<scada::DataValue>> HistoryReadRaw(
    HistoryService& history_service,
    const HistoryReadRawDetails& details) {
  struct State : public std::enable_shared_from_this<State> {
    State(HistoryService& service, HistoryReadRawDetails details)
        : service_{service}, details_{std::move(details)} {}

    promise<std::vector<scada::DataValue>> Start() {
      ReadNext();
      return promise_;
    }

    void ReadNext() {
      HistoryReadRawChunk(service_, details_)
          .then([this,
                 ref = shared_from_this()](const HistoryReadRawResult& result) {
            assert(result.status);

            values_.insert(values_.end(), result.values.begin(),
                           result.values.end());

            if (result.continuation_point.empty()) {
              promise_.resolve(std::move(values_));
              return;
            }

            details_.continuation_point = result.continuation_point;
            ReadNext();
          })
          .except([this, ref = shared_from_this()](std::exception_ptr e) {
            promise_.reject(e);
          });
    }

    HistoryService& service_;
    HistoryReadRawDetails details_;
    promise<std::vector<scada::DataValue>> promise_;

    std::vector<scada::DataValue> values_;
  };

  return std::make_shared<State>(history_service, details)->Start();
}

inline promise<scada::HistoryReadEventsResult> HistoryReadEvents(
    HistoryService& history_service,
    const NodeId& node_id,
    DateTime from,
    DateTime to,
    const EventFilter& event_filter = {}) {
  promise<scada::HistoryReadEventsResult> promise;
  history_service.HistoryReadEvents(node_id, from, to, event_filter,
                                    MakeStatusPromiseCallback(promise));
  return promise;
}

}  // namespace scada
