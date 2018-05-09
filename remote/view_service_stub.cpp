#include "remote/view_service_stub.h"

#include "common/node_id_util.h"
#include "common/scada_node_ids.h"
#include "core/status.h"
#include "core/view_service.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

using namespace std::chrono_literals;

namespace {
const auto kEventConsolidationDelay = 100ms;
}

// ViewEventQueue

void ViewEventQueue::AddModelChange(const scada::ModelChangeEvent& event) {
  for (auto& queued_event : queue_) {
    if (auto* model_change =
            std::get_if<scada::ModelChangeEvent>(&queued_event)) {
      if (model_change->node_id == event.node_id) {
        if (model_change->type_definition_id.is_null())
          model_change->type_definition_id = event.type_definition_id;
        model_change->verb |= event.verb;
        if (model_change->verb & scada::ModelChangeEvent::NodeDeleted)
          model_change->verb = scada::ModelChangeEvent::NodeDeleted;
        return;
      }
    }
  }

  queue_.emplace_back(event);
}

void ViewEventQueue::AddNodeSemanticChange(const scada::NodeId& node_id) {
  for (auto& queued_event : queue_) {
    if (auto* semantic_change_node_id =
            std::get_if<scada::NodeId>(&queued_event)) {
      if (*semantic_change_node_id == node_id)
        return;
    }
  }

  queue_.emplace_back(node_id);
}

std::vector<ViewEventQueue::Event> ViewEventQueue::GetEvents() {
  auto queue = std::move(queue_);
  queue_.clear();
  return queue;
}

// ViewServiceStub

ViewServiceStub::ViewServiceStub(ViewServiceStubContext&& context)
    : ViewServiceStubContext{std::move(context)},
      timer_{io_context_},
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

  events_.AddModelChange(event);
  ScheduleSendEvents();
}

void ViewServiceStub::OnNodeSemanticsChanged(const scada::NodeId& node_id) {
  logger_->WriteF(LogSeverity::Normal,
                  "Notification NodeSemanticsChanged [node_id=%s]",
                  NodeIdToScadaString(node_id).c_str());

  events_.AddNodeSemanticChange(node_id);
  ScheduleSendEvents();
}

void ViewServiceStub::ScheduleSendEvents() {
  timer_.StartOne(kEventConsolidationDelay, [this] { SendEvents(); });
}

void ViewServiceStub::SendEvents() {
  auto events = events_.GetEvents();
  if (events.empty())
    return;

  protocol::Message message;

  protocol::Notification* notification = nullptr;
  size_t notification_type = std::numeric_limits<size_t>::max();

  for (auto& event : events) {
    if (notification_type != event.index()) {
      notification = message.add_notifications();
      notification_type = event.index();
    }

    if (auto* model_change = std::get_if<scada::ModelChangeEvent>(&event))
      ToProto(*model_change, *notification->add_model_change());
    else if (auto* semantic_change_node_id = std::get_if<scada::NodeId>(&event))
      ToProto(*semantic_change_node_id,
              *notification->add_semantics_changed_node_id());
  }

  sender_.Send(message);
}
