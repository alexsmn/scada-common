#include "remote/node_management_stub.h"

#include "core/node_management_service.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

NodeManagementStub::NodeManagementStub(MessageSender& sender,
                                       scada::NodeManagementService& service,
                                       const scada::NodeId& user_id,
                                       std::shared_ptr<Logger> logger)
    : sender_(sender),
      service_(service),
      user_id_(user_id),
      logger_(std::move(logger)),
      weak_factory_(this) {
}

void NodeManagementStub::OnRequestReceived(const protocol::Request& request) {
  auto weak_ptr = weak_factory_.GetWeakPtr();

  if (request.has_create_node()) {
    auto& create_node = request.create_node();
    OnCreateNode(request.request_id(),
                 create_node.has_requested_node_id() ?
                     FromProto(create_node.requested_node_id()) : scada::NodeId(),
                 FromProto(create_node.parent_id()),
                 FromProto(create_node.node_class()),
                 FromProto(create_node.type_id()),
                 create_node.has_attributes() ? FromProto(create_node.attributes()) : scada::NodeAttributes());
  }

  if (request.modify_node_size() != 0) {
    std::vector<std::pair<scada::NodeId, scada::NodeAttributes>> attributes;
    attributes.reserve(request.modify_node_size());
    for (auto& modify_node : request.modify_node()) {
      attributes.emplace_back(FromProto(modify_node.node_id()),
                              FromProto(modify_node.attributes()));
    }
    OnModifyNodes(request.request_id(), attributes);
  }

  if (request.has_delete_node()) {
    auto& delete_node = request.delete_node();
    OnDeleteNode(request.request_id(),
                 FromProto(delete_node.node_id()),
                 delete_node.has_return_references() && delete_node.return_references());
  }

  if (request.has_change_password()) {
    auto& change_password = request.change_password();
    OnChangeUserPassword(request.request_id(),
                         FromProto(change_password.user_node_id()),
                         change_password.current_password(),
                         change_password.new_password());
  }

  if (request.has_add_reference())
    OnAddReference(request.request_id(), request.add_reference());

  if (request.has_delete_reference())
    OnDeleteReference(request.request_id(), request.delete_reference());
}

void NodeManagementStub::OnDeleteNode(unsigned request_id, const scada::NodeId& id, bool return_relations) {
  if (id == user_id_) {
    protocol::Message message;
    auto& response = *message.add_responses();
    response.set_request_id(request_id);
    auto& delete_node_result = *response.mutable_delete_node_result();
    ToProto(scada::StatusCode::Bad_CantDeleteOwnUser, *response.add_status());
    sender_.Send(message);
    return;
  }

  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.DeleteNode(id, return_relations,
      [weak_ptr, request_id](const scada::Status& status, const scada::NodeIdSet* references) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        auto& delete_node_result = *response.mutable_delete_node_result();
        ToProto(status, *response.add_status());
        if (references)
          ContainerToProto(*references, *delete_node_result.mutable_references());
        ptr->sender_.Send(message);
      });
}

void NodeManagementStub::OnCreateNode(unsigned request_id, const scada::NodeId& requested_id,
                                     const scada::NodeId& parent_id, scada::NodeClass node_class, const scada::NodeId& type_id,
                                     scada::NodeAttributes attributes) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.CreateNode(requested_id, parent_id, node_class, type_id,
      std::move(attributes),
      [weak_ptr, request_id](const scada::Status& status, const scada::NodeId& node_id) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        auto& create_node_result = *response.mutable_create_node_result();
        ToProto(status, *response.add_status());
        ToProto(node_id, *create_node_result.mutable_node_id());
        ptr->sender_.Send(message);
      });
}

void NodeManagementStub::OnModifyNodes(unsigned request_id, const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>& attributes) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.ModifyNodes(attributes,
      [weak_ptr, request_id](const std::vector<scada::Status>& statuses) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ContainerToProto(statuses, *response.mutable_status());
        ptr->sender_.Send(message);
      });
}

void NodeManagementStub::OnChangeUserPassword(unsigned request_id,
                                             const scada::NodeId& user_node_id,
                                             const std::string& current_pass,
                                             const std::string& new_pass) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.ChangeUserPassword(user_node_id, current_pass, new_pass,
      [weak_ptr, request_id](const scada::Status& status) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(status, *response.add_status());
        ptr->sender_.Send(message);
      });
}

void NodeManagementStub::OnAddReference(unsigned request_id, const protocol::Reference& request) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.AddReference(
      FromProto(request.reference_type_id()),
      FromProto(request.source_id()),
      FromProto(request.target_id()),
      [weak_ptr, request_id](const scada::Status& status) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(status, *response.add_status());
        ptr->sender_.Send(message);
      });
}

void NodeManagementStub::OnDeleteReference(unsigned request_id, const protocol::Reference& request) {
  auto weak_ptr = weak_factory_.GetWeakPtr();
  service_.DeleteReference(
      FromProto(request.reference_type_id()),
      FromProto(request.source_id()),
      FromProto(request.target_id()),
      [weak_ptr, request_id](const scada::Status& status) {
        auto ptr = weak_ptr.get();
        if (!ptr)
          return;

        protocol::Message message;
        auto& response = *message.add_responses();
        response.set_request_id(request_id);
        ToProto(status, *response.add_status());
        ptr->sender_.Send(message);
      });
}
