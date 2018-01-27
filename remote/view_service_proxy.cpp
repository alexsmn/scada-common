#include "remote/view_service_proxy.h"

#include "base/logger.h"
#include "core/standard_node_ids.h"
#include "core/status.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

// ViewServiceProxy

ViewServiceProxy::ViewServiceProxy(std::shared_ptr<Logger> logger)
    : logger_(std::move(logger)), sender_(nullptr) {}

void ViewServiceProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void ViewServiceProxy::OnChannelClosed() {
  sender_ = nullptr;
}

void ViewServiceProxy::OnNotification(
    const protocol::Notification& notification) {
  // Notification should contain only one type changes.
  // But it can be addition on complex object.

  for (auto& proto_node_id : notification.deleted_node_id()) {
    const auto node_id = FromProto(proto_node_id);
    FOR_EACH_OBSERVER(scada::ViewEvents, events_, OnNodeDeleted(node_id));
  }

  for (auto& proto_node_id : notification.semantics_changed_node_id()) {
    const auto node_id = FromProto(proto_node_id);
    FOR_EACH_OBSERVER(scada::ViewEvents, events_,
                      OnNodeSemanticsChanged(node_id));
  }

  for (auto& proto_node_id : notification.added_node_id()) {
    const auto node_id = FromProto(proto_node_id);
    FOR_EACH_OBSERVER(scada::ViewEvents, events_, OnNodeAdded(node_id));
  }

  for (auto& proto_node_id : notification.added_reference_node_id()) {
    const auto node_id = FromProto(proto_node_id);
    FOR_EACH_OBSERVER(scada::ViewEvents, events_, OnReferenceAdded(node_id));
  }

  for (auto& proto_node_id : notification.deleted_reference_node_id()) {
    const auto node_id = FromProto(proto_node_id);
    FOR_EACH_OBSERVER(scada::ViewEvents, events_, OnReferenceDeleted(node_id));
  }
}

void ViewServiceProxy::Browse(
    const std::vector<scada::BrowseDescription>& nodes,
    const scada::BrowseCallback& callback) {
  assert(sender_);

  protocol::Request request;
  auto& browse = *request.mutable_browse();
  for (auto& node : nodes) {
    auto& browse_node = *browse.add_nodes();
    ToProto(node.node_id, *browse_node.mutable_node_id());
    if (node.direction != scada::BrowseDirection::Both)
      browse_node.set_direction(ToProto(node.direction));
    ToProto(node.reference_type_id, *browse_node.mutable_reference_type_id());
    if (node.include_subtypes)
      browse_node.set_include_subtypes(true);
  }

  sender_->Request(request, [callback](const protocol::Response& response) {
    auto& browse = response.browse();
    callback(FromProto(response.status()),
             VectorFromProto<scada::BrowseResult>(browse.results()));
  });
}

void ViewServiceProxy::TranslateBrowsePath(
    const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path,
    const scada::TranslateBrowsePathCallback& callback) {
  callback(scada::StatusCode::Bad, {}, 0);
}

void ViewServiceProxy::Subscribe(scada::ViewEvents& events) {
  events_.AddObserver(&events);
}

void ViewServiceProxy::Unsubscribe(scada::ViewEvents& events) {
  events_.RemoveObserver(&events);
}
