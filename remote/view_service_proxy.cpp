#include "remote/view_service_proxy.h"

#include "base/logger.h"
#include "base/strings/sys_string_conversions.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

// ViewServiceProxy

ViewServiceProxy::ViewServiceProxy(std::shared_ptr<Logger> logger)
    : logger_{std::move(logger)} {
}

ViewServiceProxy::~ViewServiceProxy() {
}

void ViewServiceProxy::Subscribe(scada::ViewEvents& events) {
  view_events_.AddObserver(&events);
}

void ViewServiceProxy::Unsubscribe(scada::ViewEvents& events) {
  view_events_.RemoveObserver(&events);
}

void ViewServiceProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void ViewServiceProxy::OnChannelClosed() {
  sender_ = nullptr;
}

void ViewServiceProxy::Browse(const std::vector<scada::BrowseDescription>& nodes, const scada::BrowseCallback& callback) {
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
    callback(
        FromProto(browse.status()),
        VectorFromProto<scada::BrowseResult>(browse.results())
    );
  });
}

void ViewServiceProxy::TranslateBrowsePath(const scada::NodeId& starting_node_id,
    const scada::RelativePath& relative_path, const scada::TranslateBrowsePathCallback& callback) {
  assert(false);
  callback(scada::StatusCode::Bad, {}, 0);
}


void LogNodes(const Logger& logger, const char* prefix, const ::google::protobuf::RepeatedPtrField<::protocol::NodeId>& node_ids) {
  for (auto& proto_node_id : node_ids) {
    auto node_id = FromProto(proto_node_id);
    logger.WriteF(LogSeverity::Normal, "%s [node_id=%s]", prefix, node_id.ToString().c_str());
  }
}

void LogReferences(const Logger& logger, const char* prefix, const ::google::protobuf::RepeatedPtrField<::protocol::Reference>& references) {
  for (auto& reference : references) {
    auto reference_type_id = FromProto(reference.reference_type_id());
    auto source_id = FromProto(reference.source_id());
    auto target_id = FromProto(reference.target_id());

    logger.WriteF(LogSeverity::Normal,
        "%s [reference_type_id=%s, source_id=%s, target_id=%s]",
        prefix,
        reference_type_id.ToString().c_str(),
        source_id.ToString().c_str(),
        target_id.ToString().c_str());
  }
}

void ViewServiceProxy::OnNotification(const protocol::Notification& notification) {
  // Notification should contain only one type changes.
  // But it can be addition on complex object.

  // Logging.

  LogNodes(*logger_, "Notification NodeAdded", notification.added_node_id());
  LogNodes(*logger_, "Notification NodeDeleted", notification.deleted_node_id());
  LogNodes(*logger_, "Notification NodeSemanticsChanged", notification.semantics_changed_node_id());
  LogReferences(*logger_, "Notification ReferenceAdded", notification.added_references());
  LogReferences(*logger_, "Notification ReferenceDeleted", notification.deleted_references());

  // TODO: Redesign.
  /*configuration_builder_.CreateNodes(
      BrowseVectorFromProto<scada::BrowseNode>(notification.added_nodes()),
      VectorFromProto<scada::BrowseReference>(notification.added_references()));*/

  // TODO: Redesign.
  /*if (notification.modified_nodes_size() != 0) {
    // TODO: Log.
    std::vector<std::pair<scada::NodeId, scada::NodeAttributes>> attributes;
    attributes.reserve(notification.modified_nodes_size());
    for (auto& modified_node : notification.modified_nodes()) {
      attributes.emplace_back(FromProto(modified_node.node_id()),
                              FromProto(modified_node.attributes()));
    }
    address_space_.ModifyNodes(attributes);
  }*/

  /*for (auto& reference : notification.deleted_references()) {
    scada::BrowseReference ref{
        FromProto(reference.reference_type_id()),
        FromProto(reference.source_id()),
        FromProto(reference.target_id()),
    };
    logger_->WriteF(LogSeverity::Normal, "DeleteReference [reference_type_id=%s, source_id=%s, target_id=%s]",
        ref.reference_type_id.ToString().c_str(),
        ref.source_id.ToString().c_str(),
        ref.target_id.ToString().c_str());
    FOR_EACH_OBSERVER(scada::ViewEvents, view_events_, OnReferenceDeleted(ref));
  }

  for (auto& deleted_id : notification.deleted_node_ids()) {
    auto node_id = FromProto(deleted_id);
    logger_->WriteF(LogSeverity::Normal, "DeleteNode [node_id=%s]", node_id.ToString().c_str());
    FOR_EACH_OBSERVER(scada::ViewEvents, view_events_, OnNodeDeleted(node_id));
  }*/
}

