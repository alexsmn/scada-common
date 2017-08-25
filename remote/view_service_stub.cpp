#include "remote/view_service_stub.h"

#include "core/view_service.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"
#include "remote/message_sender.h"

ViewServiceStub::ViewServiceStub(MessageSender& sender,
                                 scada::ViewService& service,
                                 std::shared_ptr<Logger> logger)
    : sender_(sender),
      service_(service),
      logger_(std::move(logger)),
      weak_factory_(this) {
  service_.Subscribe(*this);
}

ViewServiceStub::~ViewServiceStub() {
  service_.Unsubscribe(*this);
}

void ViewServiceStub::OnRequestReceived(const protocol::Request& request) {
  if (request.has_browse()) {
    auto& proto_nodes = request.browse().nodes();
    std::vector<scada::BrowseDescription> nodes;
    nodes.reserve(proto_nodes.size());
    for (auto& proto_node : proto_nodes) {
      nodes.push_back({
          FromProto(proto_node.node_id()),
          proto_node.has_direction() ? FromProto(proto_node.direction()) : scada::BrowseDirection::Both,
          proto_node.has_reference_type_id() ? FromProto(proto_node.reference_type_id()) : scada::NodeId{},
          proto_node.include_subtypes(),
      });
    }
    OnBrowse(request.request_id(), std::move(nodes));
  }
}

void ViewServiceStub::OnBrowse(unsigned request_id, const std::vector<scada::BrowseDescription>& nodes) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.Browse(nodes, [=](const scada::Status& status, std::vector<scada::BrowseResult> results) {
    if (!weak_ptr)
      return;

    protocol::Message message;
    auto& response = *message.add_responses();
    response.set_request_id(request_id);
    auto& browse = *response.mutable_browse();
    ToProto(status, *browse.mutable_status());
    if (status)
      ToProto(results, *browse.mutable_results());

    sender_.Send(message);
  });
}

void ViewServiceStub::OnNodeAdded(const scada::NodeId& node_id) {
  logger_->WriteF(LogSeverity::Normal, "Notification NodeCreated [node_id=%s]",
      node_id.ToString().c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();
  ToProto(node_id, *notification.add_added_node_id());

  sender_.Send(message);
}

void ViewServiceStub::OnNodeDeleted(const scada::NodeId& node_id) {
  logger_->WriteF(LogSeverity::Normal, "Notification NodeDeleted [node_id=%s]",
      node_id.ToString().c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();
  ToProto(node_id, *notification.add_deleted_node_id());
  sender_.Send(message);
}

void ViewServiceStub::OnNodeModified(const scada::NodeId& node_id, const scada::PropertyIds& property_ids) {
  logger_->WriteF(LogSeverity::Normal, "Notification NodeModified [node_id=%s]",
      node_id.ToString().c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();
  ToProto(node_id, *notification.add_semantics_changed_node_id());
  sender_.Send(message);
}

void ViewServiceStub::OnReferenceAdded(const scada::BrowseReference& reference) {
  logger_->WriteF(LogSeverity::Normal,
      "Notification ReferenceAdded [reference_type_id=%s, source_id=%s, target_id=%s]",
      reference.reference_type_id.ToString().c_str(),
      reference.source_id.ToString().c_str(),
      reference.target_id.ToString().c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();
  auto& ref = *notification.add_added_references();
  ToProto(reference.reference_type_id, *ref.mutable_reference_type_id());
  ToProto(reference.source_id, *ref.mutable_source_id());
  ToProto(reference.target_id, *ref.mutable_target_id());
  sender_.Send(message);
}

void ViewServiceStub::OnReferenceDeleted(const scada::BrowseReference& reference) {
  logger_->WriteF(LogSeverity::Normal, "Notification ReferenceDeleted [reference_type_id=%s, source_id=%s, target_id=%s]",
      reference.reference_type_id.ToString().c_str(),
      reference.source_id.ToString().c_str(),
      reference.target_id.ToString().c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();
  auto& ref = *notification.add_deleted_references();
  ToProto(reference.reference_type_id, *ref.mutable_reference_type_id());
  ToProto(reference.source_id, *ref.mutable_source_id());
  ToProto(reference.target_id, *ref.mutable_target_id());
  sender_.Send(message);
}
