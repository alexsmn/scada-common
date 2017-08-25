#pragma once

#include "core/configuration_types.h"
#include "core/history_service.h"

class MessageSender;

class HistoryProxy : public scada::HistoryService {
 public:
  HistoryProxy();

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();

  // HistoryService
  virtual void QueryItemInfos(const scada::ItemInfosCallback& callback) override;
  virtual void WriteItemInfo(const scada::ItemInfo& info) override;
  virtual void HistoryRead(const scada::ReadValueId& read_value_id, base::Time from, base::Time to,
                           const scada::Filter& filter, const scada::HistoryReadCallback& callback) override;
  virtual void WriteEvent(const scada::Event& event) override;
  virtual void AcknowledgeEvent(unsigned ack_id, base::Time time,
                                const scada::NodeId& user_node_id) override;

 private:
  MessageSender* sender_;
};
