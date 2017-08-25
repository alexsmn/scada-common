#pragma once

#include <functional>

#include "base/strings/string_piece.h"
#include "core/attribute_ids.h"
#include "core/data_value.h"

typedef int MonitoredItemId;

struct SubscriptionParams {
  base::TimeDelta update_rate;
};

struct MonitoredItemParams {
  MonitoredItemId handle;
  base::StringPiece16 item_id;
};

/*namespace scada {

class Status;

struct Notification {
};

struct DataChangeNotification : Notification {
};

struct EventNotification : Notification {
};

struct StatusChangeNotification : Notification {
};

class SubscriptionService {
 public:
  virtual ~SubscriptionService() {}

  using PublishCallback = std::function<void(const Status& status, const std::vector<Notification>& notifications)>;

  virtual void Publish(PublishCallback callback) = 0;
};

} // namespace scada*/
