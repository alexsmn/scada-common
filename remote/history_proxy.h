#pragma once

#include "core/configuration_types.h"
#include "core/history_service.h"

class MessageSender;

class HistoryProxy : public scada::HistoryService {
 public:
  HistoryProxy();

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();

  // scada::HistoryService
  virtual void HistoryReadRaw(
      const scada::NodeId& node_id,
      base::Time from,
      base::Time to,
      const scada::ContinuationPoint& continuation_point,
      const scada::HistoryReadRawCallback& callback) override;
  virtual void HistoryReadEvents(
      const scada::NodeId& node_id,
      base::Time from,
      base::Time to,
      const scada::EventFilter& filter,
      const scada::HistoryReadEventsCallback& callback) override;

 private:
  MessageSender* sender_ = nullptr;
};
