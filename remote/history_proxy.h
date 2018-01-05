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
  virtual void HistoryRead(const scada::ReadValueId& read_value_id, base::Time from, base::Time to,
                           const scada::Filter& filter, const scada::HistoryReadCallback& callback) override;

 private:
  MessageSender* sender_;
};
