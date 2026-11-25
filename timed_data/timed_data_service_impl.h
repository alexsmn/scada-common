#pragma once

#include "base/cancelation.h"
#include "base/timed_cache.h"
#include "timed_data/timed_data_context.h"
#include "timed_data/timed_data_service.h"

class TimedDataImpl;
class AliasTimedData;

class TimedDataServiceImpl final : private TimedDataContext,
                                   public TimedDataService {
 public:
  explicit TimedDataServiceImpl(TimedDataContext&& context);
  explicit TimedDataServiceImpl(CoroutineTimedDataContext&& context);
  virtual ~TimedDataServiceImpl();

  virtual std::shared_ptr<TimedData> GetFormulaTimedData(
      std::string_view formula,
      const scada::AggregateFilter& aggregation) override;
  virtual std::shared_ptr<TimedData> GetNodeTimedData(
      const scada::NodeId& node_id,
      const scada::AggregateFilter& aggregation) override;

  // True while any live timed data is still waiting on history it asked for.
  //
  // Exists for tooling that must render a *complete* view — the screenshot
  // generator waits on this instead of pumping the event loop for a fixed
  // duration and hoping. A fixed pump is a shared budget: a run capturing many
  // windows gives each one less settling time, so the same view rendered with
  // or without its trends depending on what else the run contained.
  bool HasPendingHistory() const;

 private:
  std::shared_ptr<TimedData> GetAliasTimedData(
      std::string_view alias,
      const scada::AggregateFilter& aggregation);

  // The node `name` names, when it is a node id rather than an alias: the
  // self-contained OPC UA form (`{nsu=<uri>;s=…}`, `i=2258`), resolved through
  // the connection's namespace table, or the model's `TS.1379`. A null NodeId
  // when it is a node id that names nothing this connection can resolve;
  // nullopt when it is not a node id at all.
  std::optional<scada::NodeId> ResolveNodeId(std::string_view name) const;

  // An expression operand with its braces dropped when it is a node id.
  std::string_view UnbraceOperand(std::string_view name) const;

  TimedCache<std::pair<scada::NodeId, scada::AggregateFilter>,
             std::shared_ptr<TimedDataImpl>>
      node_id_cache_;
  TimedCache<std::pair<std::string, scada::AggregateFilter>,
             std::shared_ptr<AliasTimedData>>
      alias_cache_;

  const std::shared_ptr<TimedData> null_timed_data_;

  Cancelation cancelation_;
};
