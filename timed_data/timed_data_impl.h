#pragma once

#include "base/memory/weak_ptr.h"
#include "base/nested_logger.h"
#include "core/monitored_item.h"
#include "core/history_service.h"
#include "common/timed_data/timed_data.h"
#include "common/timed_data/timed_data_context.h"
#include "common/event_observer.h"
#include "common/node_ref_observer.h"

namespace scada {
class EventService;
class MonitoredItemService;
}

namespace rt {

class TimedDataImpl : public TimedData,
                      private TimedDataContext,
                      private NodeRefObserver,
                      private events::EventObserver {
 public:
  TimedDataImpl(const scada::NodeId& node_id, const TimedDataContext& context,
                std::shared_ptr<const Logger> parent_logger);
  TimedDataImpl(const NodeRef& node, const TimedDataContext& context,
                std::shared_ptr<const Logger> parent_logger);
  virtual ~TimedDataImpl();

  // TimedData overrides
  virtual std::string GetFormula(bool aliases) const override;
  virtual base::string16 GetTitle() const override;
  virtual NodeRef GetNode() const override;
  virtual void Write(double value, const scada::WriteFlags& flags,
                     const StatusCallback& callback) const override;
  virtual void Call(const scada::NodeId& method_id, const std::vector<scada::Variant>& arguments,
                    const StatusCallback& callback) const override;
  virtual const events::EventSet* GetEvents() const override;
  virtual void Acknowledge() override;

 protected:
  // TimedData
  virtual void OnFromChanged() override;

 private:
  void SetNode(NodeRef node);

  void HistoryRead();

  void OnQueryValuesComplete(scada::Status status, base::Time queried_from, base::Time queried_to,
                             scada::QueryValuesResults results);

  void OnChannelData(const scada::DataValue& data_value);

  // scada::NodeObserver
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeSemanticChanged(const scada::NodeId& node_id) override;

  // events::EventObserver
  virtual void OnItemEventsChanged(const scada::NodeId& item_id, const events::EventSet& events) override;

  NestedLogger logger_;

  NodeRef node_;
  std::unique_ptr<scada::MonitoredItem> monitored_value_;

  bool querying_ = false;

  base::WeakPtrFactory<TimedDataImpl> weak_ptr_factory_{this};
};

} // namespace rt
