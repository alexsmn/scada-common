#include "remote/view_service_proxy.h"

#include "base/logger.h"
#include "common/node_id_util.h"
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

  for (auto& model_change : notification.model_change()) {
    const auto event = FromProto(model_change);
    for (auto& e : events_)
      e.OnModelChanged(event);
  }

  for (auto& semantics_changed_node_id :
       notification.semantics_changed_node_id()) {
    const auto node_id = FromProto(semantics_changed_node_id);
    for (auto& e : events_)
      e.OnNodeSemanticsChanged(node_id);
  }
}

void ViewServiceProxy::Browse(
    const std::vector<scada::BrowseDescription>& nodes,
    const scada::BrowseCallback& callback) {
  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

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
