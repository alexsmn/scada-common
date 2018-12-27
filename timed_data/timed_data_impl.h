#pragma once

#include "base/memory/weak_ptr.h"
#include "base/nested_logger.h"
#include "common/event_observer.h"
#include "common/node_observer.h"
#include "core/aggregate_filter.h"
#include "core/history_service.h"
#include "core/monitored_item.h"
#include "timed_data/base_timed_data.h"
#include "timed_data/timed_data_context.h"

namespace scada {
class EventService;
class MonitoredItemService;
}  // namespace scada

namespace rt {

class ScopedContinuationPoint;

class TimedDataImpl : private TimedDataContext,
                      public BaseTimedData,
                      private NodeRefObserver,
                      private events::EventObserver {
 public:
  TimedDataImpl(NodeRef node,
                scada::AggregateFilter aggregate_filter,
                TimedDataContext context,
                std::shared_ptr<const Logger> logger);
  virtual ~TimedDataImpl();

  // TimedData overrides
  virtual std::string GetFormula(bool aliases) const override;
  virtual scada::LocalizedText GetTitle() const override;
  virtual NodeRef GetNode() const override;
  virtual void Write(double value,
                     const scada::NodeId& user_id,
                     const scada::WriteFlags& flags,
                     const StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id,
                    const std::vector<scada::Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) const override;
  virtual const events::EventSet* GetEvents() const override;
  virtual void Acknowledge() override;

 protected:
  // TimedData
  virtual void OnFromChanged() override;

 private:
  void SetNode(const NodeRef& node);

  void QueryValues(ScopedContinuationPoint&& continuation_point);

  void OnHistoryReadRawComplete(base::Time queried_from,
                                base::Time queried_to,
                                std::vector<scada::DataValue>&& values,
                                scada::ByteString&& continuation_point);

  void OnChannelData(const scada::DataValue& data_value);

  // NodeRefObserver
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  // events::EventObserver
  virtual void OnItemEventsChanged(const scada::NodeId& node_id,
                                   const events::EventSet& events) override;

  const std::shared_ptr<const Logger> logger_;
  const scada::AggregateFilter aggregate_filter_;

  NodeRef node_;
  std::unique_ptr<scada::MonitoredItem> monitored_value_;

  bool querying_ = false;

  base::WeakPtrFactory<TimedDataImpl> weak_ptr_factory_{this};
};

}  // namespace rt
