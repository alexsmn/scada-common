#include "remote/subscription_proxy.h"

#include <cassert>

#include "base/strings/sys_string_conversions.h"
#include "core/monitored_item.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

using namespace scada;

namespace {
const auto kInvalidIndex = std::numeric_limits<std::size_t>::max();
}

// SubscriptionProxy::MonitoredItemProxy

class SubscriptionProxy::MonitoredItemProxy : public MonitoredItem {
 public:
  MonitoredItemProxy(SubscriptionProxy& subscription,
                     const scada::ReadValueId& read_value_id);
  virtual ~MonitoredItemProxy();

  virtual void Subscribe() override;

  void OnChannelOpened();
  void OnChannelClosed();

  void UpdateAndForwardData(const DataValue& value);

 private:
  void CreateStub();
  void DeleteStub();

  void OnCreateMonitoredItemResult(const Status& status, int monitored_item_id);

  void UpdateQualifier(unsigned remove, unsigned add);

  SubscriptionProxy& subscription_;
  const scada::ReadValueId read_value_id_;

  MonitoredItemId monitored_item_id_;

  enum State { DELETED, CREATING, CREATED };
  State state_ = DELETED;

  DataValue current_data_;

  std::shared_ptr<bool> cancelation_;
};

SubscriptionProxy::MonitoredItemProxy::MonitoredItemProxy(
    SubscriptionProxy& subscription,
    const scada::ReadValueId& read_value_id)
    : subscription_(std::move(subscription)),
      read_value_id_(std::move(read_value_id)),
      cancelation_(std::make_shared<bool>(false)) {}

SubscriptionProxy::MonitoredItemProxy::~MonitoredItemProxy() {
  if (state_ == CREATED)
    DeleteStub();

  subscription_.RemoveMonitoredItem(*this);
}

void SubscriptionProxy::MonitoredItemProxy::Subscribe() {
  assert(state_ == DELETED);

  subscription_.AddMonitoredItem(*this);

  if (!current_data_.is_null())
    ForwardData(current_data_);
}

void SubscriptionProxy::MonitoredItemProxy::CreateStub() {
  assert(state_ == DELETED);
  state_ = CREATING;

  protocol::Request request;
  auto& create_monitored_item = *request.mutable_create_monitored_item();
  create_monitored_item.set_subscription_id(subscription_.subscription_id_);
  ToProto(read_value_id_.node_id, *create_monitored_item.mutable_node_id());
  create_monitored_item.set_attribute_id(
      static_cast<protocol::AttributeId>(read_value_id_.attribute_id));

  std::weak_ptr<bool> cancelation = cancelation_;
  subscription_.sender_->Request(
      request, [this, cancelation](const protocol::Response& response) {
        if (cancelation.expired())
          return;
        int monitored_item_id = 0;
        if (response.has_create_monitored_item_result()) {
          auto& m = response.create_monitored_item_result();
          monitored_item_id = m.monitored_item_id();
        }
        OnCreateMonitoredItemResult(FromProto(response.status()),
                                    monitored_item_id);
      });
}

void SubscriptionProxy::MonitoredItemProxy::DeleteStub() {
  assert(state_ == CREATING || state_ == CREATED);

  state_ = DELETED;

  auto monitored_item_id = monitored_item_id_;
  subscription_.monitored_item_ids_.erase(monitored_item_id_);
  monitored_item_id_ = 0;

  protocol::Message message;
  auto& request = *message.add_requests();
  request.set_request_id(0);
  auto& delete_monitored_item = *request.mutable_delete_monitored_item();
  delete_monitored_item.set_subscription_id(subscription_.subscription_id_);
  delete_monitored_item.set_monitored_item_id(monitored_item_id);
  subscription_.sender_->Send(message);
}

void SubscriptionProxy::MonitoredItemProxy::OnChannelOpened() {
  assert(state_ == DELETED);

  DataValue tvq;
  tvq.server_timestamp = base::Time::Now();
  tvq.qualifier = Qualifier::OFFLINE;
  current_data_ = tvq;
  ForwardData(tvq);

  CreateStub();
}

void SubscriptionProxy::MonitoredItemProxy::OnChannelClosed() {
  state_ = DELETED;

  subscription_.monitored_item_ids_.erase(monitored_item_id_);
  monitored_item_id_ = 0;

  UpdateQualifier(0, Qualifier::OFFLINE);
}

void SubscriptionProxy::MonitoredItemProxy::OnCreateMonitoredItemResult(
    const Status& status,
    int monitored_item_id) {
  if (!status) {
    state_ = DELETED;
    subscription_.RemoveMonitoredItem(*this);
    UpdateQualifier(0, Qualifier::FAILED);
    return;
  }

  state_ = CREATED;
  monitored_item_id_ = monitored_item_id;
  subscription_.monitored_item_ids_[monitored_item_id_] = this;
}

void SubscriptionProxy::MonitoredItemProxy::UpdateAndForwardData(
    const DataValue& value) {
  if (value == current_data_)
    return;

  // Update current value only with latest time.
  if (IsUpdate(current_data_, value))
    current_data_ = value;

  // But any data shall be notified to delegate.
  ForwardData(value);
}

void SubscriptionProxy::MonitoredItemProxy::UpdateQualifier(unsigned remove,
                                                            unsigned add) {
  Qualifier new_qualifier = current_data_.qualifier;
  new_qualifier.Update(remove, add);
  if (current_data_.qualifier == new_qualifier)
    return;

  current_data_.qualifier = new_qualifier;

  auto data = current_data_;
  ForwardData(data);
}

// SubscriptionProxy

SubscriptionProxy::SubscriptionProxy(const SubscriptionParams& params)
    : sender_(nullptr),
      state_(DELETED),
      subscription_id_(0),
      cancelation_(std::make_shared<bool>(false)) {}

SubscriptionProxy::~SubscriptionProxy() {
  assert(monitored_items_.empty());

  if (state_ == CREATED) {
    protocol::Message message;
    auto& request = *message.add_requests();
    request.set_request_id(0);
    auto& delete_subscription = *request.mutable_delete_subscription();
    delete_subscription.set_subscription_id(subscription_id_);
    sender_->Send(message);
  }
}

std::unique_ptr<MonitoredItem> SubscriptionProxy::CreateMonitoredItem(
    const scada::ReadValueId& read_value_id) {
  assert(read_value_id.attribute_id == AttributeId::EventNotifier ||
         !read_value_id.node_id.is_null());
  return std::unique_ptr<MonitoredItem>(
      new MonitoredItemProxy(*this, read_value_id));
}

void SubscriptionProxy::AddMonitoredItem(MonitoredItemProxy& item) {
  monitored_items_.insert(&item);

  if (state_ == CREATED)
    item.OnChannelOpened();
}

void SubscriptionProxy::RemoveMonitoredItem(MonitoredItemProxy& item) {
  monitored_items_.erase(&item);
}

void SubscriptionProxy::OnDataChange(int monitored_item_id,
                                     const DataValue& data_value) {
  auto i = monitored_item_ids_.find(monitored_item_id);
  if (i != monitored_item_ids_.end())
    i->second->UpdateAndForwardData(data_value);
}

void SubscriptionProxy::OnEvent(int monitored_item_id,
                                const scada::Status& status,
                                const Event& event) {
  auto i = monitored_item_ids_.find(monitored_item_id);
  if (i != monitored_item_ids_.end())
    i->second->ForwardEvent(status, event);
}

void SubscriptionProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;

  assert(state_ == DELETED);
  state_ = CREATING;

  protocol::Request request;
  auto& create_subscription = *request.mutable_create_subscription();

  std::weak_ptr<bool> cancelation = cancelation_;
  sender_->Request(request, [this,
                             cancelation](const protocol::Response& response) {
    if (cancelation.expired())
      return;
    int subscription_id = 0;
    if (response.has_create_subscription_result()) {
      auto& m = response.create_subscription_result();
      subscription_id = m.subscription_id();
    }
    OnCreateSubscriptionResult(FromProto(response.status()), subscription_id);
  });
}

void SubscriptionProxy::OnChannelClosed() {
  sender_ = nullptr;

  if (state_ == DELETED)
    return;

  state_ = DELETED;
  subscription_id_ = 0;

  for (auto* channel : monitored_items_)
    channel->OnChannelClosed();
}

void SubscriptionProxy::OnCreateSubscriptionResult(const Status& status,
                                                   int subscription_id) {
  if (state_ != CREATING)
    return;

  if (!status) {
    state_ = DELETED;
    return;
  }

  state_ = CREATED;
  subscription_id_ = subscription_id;

  for (auto* channel : monitored_items_)
    channel->OnChannelOpened();
}
