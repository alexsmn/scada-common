#include "remote/view_service_stub.h"

#include "common/node_id_util.h"
#include "common/scada_node_ids.h"
#include "core/status.h"
#include "core/view_service.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

// ViewServiceStub

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
          proto_node.has_direction() ? FromProto(proto_node.direction())
                                     : scada::BrowseDirection::Both,
          proto_node.has_reference_type_id()
              ? FromProto(proto_node.reference_type_id())
              : scada::NodeId{},
          proto_node.include_subtypes(),
      });
    }
    OnBrowse(request.request_id(), std::move(nodes));
  }
}

void ViewServiceStub::OnBrowse(
    unsigned request_id,
    const std::vector<scada::BrowseDescription>& nodes) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.Browse(nodes, [=](const scada::Status& status,
                             std::vector<scada::BrowseResult> results) {
    if (!weak_ptr)
      return;

    protocol::Message message;
    auto& response = *message.add_responses();
    response.set_request_id(request_id);
    ToProto(status, *response.mutable_status());
    auto& browse = *response.mutable_browse();
    if (status)
      ToProto(results, *browse.mutable_results());

    sender_.Send(message);
  });
}

void ViewServiceStub::OnModelChanged(const scada::ModelChangeEvent& event) {
  logger_->WriteF(LogSeverity::Normal, "Notification ModelChanged [node_id=%s]",
                  NodeIdToScadaString(event.node_id).c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();

  if (event.verb & scada::ModelChangeEvent::NodeAdded)
    ToProto(event.node_id, *notification.add_added_node_id());
  if (event.verb & scada::ModelChangeEvent::NodeDeleted)
    ToProto(event.node_id, *notification.add_deleted_node_id());
  if (event.verb & scada::ModelChangeEvent::ReferenceAdded)
    ToProto(event.node_id, *notification.add_added_reference_node_id());
  if (event.verb & scada::ModelChangeEvent::ReferenceDeleted)
    ToProto(event.node_id, *notification.add_deleted_reference_node_id());

  sender_.Send(message);
}

void ViewServiceStub::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  logger_->WriteF(LogSeverity::Normal,
                  "Notification NodeSemanticsChanged [node_id=%s]",
                  NodeIdToScadaString(node_id).c_str());

  protocol::Message message;
  auto& notification = *message.add_notifications();
  ToProto(node_id, *notification.add_semantics_changed_node_id());
  sender_.Send(message);
}
