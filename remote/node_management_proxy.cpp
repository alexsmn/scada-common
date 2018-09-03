#include "remote/node_management_proxy.h"

#include "base/logger.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "common/node_id_util.h"
#include "remote/message_sender.h"
#include "remote/protocol.h"
#include "remote/protocol_utils.h"

NodeManagementProxy::NodeManagementProxy(std::shared_ptr<Logger> logger)
    : logger_(std::move(logger)) {}

void NodeManagementProxy::OnChannelOpened(MessageSender& sender) {
  sender_ = &sender;
}

void NodeManagementProxy::OnChannelClosed() {
  sender_ = nullptr;
}

void NodeManagementProxy::CreateNode(const scada::NodeId& requested_id,
                                     const scada::NodeId& parent_id,
                                     scada::NodeClass node_class,
                                     const scada::NodeId& type_id,
                                     scada::NodeAttributes attributes,
                                     const CreateNodeCallback& callback) {
  logger().WriteF(LogSeverity::Normal,
                  "CreateNode request [requested_id=%s, node_class=%s, "
                  "parent_id=%s, type_id=%s]",
                  NodeIdToScadaString(requested_id).c_str(),
                  ToString(node_class).c_str(),
                  NodeIdToScadaString(parent_id).c_str(),
                  NodeIdToScadaString(type_id).c_str());

  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  protocol::Request request;
  auto& create_node = *request.mutable_create_node();
  create_node.set_node_class(ToProto(node_class));
  if (!requested_id.is_null())
    ToProto(requested_id, *create_node.mutable_requested_node_id());
  ToProto(parent_id, *create_node.mutable_parent_id());
  ToProto(type_id, *create_node.mutable_type_definition_id());
  if (!attributes.empty())
    ToProto(std::move(attributes), *create_node.mutable_attributes());

  sender_->Request(request, [this,
                             callback](const protocol::Response& response) {
    auto status = FromProto(response.status());
    scada::NodeId node_id;
    if (response.has_create_node_result())
      node_id = FromProto(response.create_node_result().node_id());

    logger().WriteF(
        LogSeverity::Normal, "CreateNode response [status='%ls', node_id=%s]",
        ToString16(status).c_str(), NodeIdToScadaString(node_id).c_str());

    if (callback)
      callback(std::move(status), node_id);
  });
}

void NodeManagementProxy::ModifyNodes(
    const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>&
        attributes,
    const scada::ModifyNodesCallback& callback) {
  logger().WriteF(LogSeverity::Normal, "ModifyNodes request [count=%Iu]",
                  attributes.size());

  // TODO: Log.

  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected, {});

  protocol::Request request;
  request.mutable_modify_node()->Reserve(attributes.size());
  for (auto& [node_id, attribute] : attributes) {
    assert(!node_id.is_null());
    // Attributes can be empty when writing a null value.
    // TODO: Fix it.
    // assert(!attribute.empty());
    auto& modify_node = *request.add_modify_node();
    ToProto(node_id, *modify_node.mutable_node_id());
    ToProto(std::move(attribute), *modify_node.mutable_attributes());
  }

  sender_->Request(request, [this,
                             callback](const protocol::Response& response) {
    auto status = FromProto(response.status());
    auto results =
        VectorFromProto<scada::Status>(response.modify_node_result());

    logger().WriteF(LogSeverity::Normal, "ModifyNodes response [status='%ls']",
                    ToString16(status).c_str());
    // TODO: Log.

    if (callback)
      callback(std::move(status), std::move(results));
  });
}

void NodeManagementProxy::DeleteNode(
    const scada::NodeId& node_id,
    bool return_relations,
    const scada::DeleteNodeCallback& callback) {
  logger().WriteF(LogSeverity::Normal, "DeleteNode request [node_id=%s]",
                  NodeIdToScadaString(node_id).c_str());

  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected, nullptr);

  protocol::Request request;
  auto& delete_node = *request.mutable_delete_node();
  ToProto(node_id, *delete_node.mutable_node_id());
  if (return_relations)
    delete_node.set_return_references(true);

  sender_->Request(request, [this,
                             callback](const protocol::Response& response) {
    auto status = FromProto(response.status());
    std::set<scada::NodeId> references;
    if (response.has_delete_node_result())
      references = SetFromProto<scada::NodeId>(
          response.delete_node_result().references());

    logger().WriteF(LogSeverity::Normal, "DeleteNode response [status='%ls']",
                    ToString16(status).c_str());

    if (!callback)
      return;

    callback(status, &references);
  });
}

void NodeManagementProxy::ChangeUserPassword(
    const scada::NodeId& user_node_id,
    const scada::LocalizedText& current_password,
    const scada::LocalizedText& new_password,
    const scada::StatusCallback& callback) {
  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected);

  protocol::Request request;
  auto& change_password = *request.mutable_change_password();
  ToProto(user_node_id, *change_password.mutable_user_node_id());
  change_password.set_current_password_utf8(
      base::UTF16ToUTF8(current_password));
  change_password.set_new_password_utf8(base::UTF16ToUTF8(new_password));

  sender_->Request(request,
                   [this, callback](const protocol::Response& response) {
                     if (callback)
                       callback(FromProto(response.status()));
                   });
}

void NodeManagementProxy::AddReference(const scada::NodeId& reference_type_id,
                                       const scada::NodeId& source_id,
                                       const scada::NodeId& target_id,
                                       const scada::StatusCallback& callback) {
  logger().WriteF(
      LogSeverity::Normal,
      "AddReference request [reference_type_id=%s, source_id=%s, target_id=%s]",
      NodeIdToScadaString(reference_type_id).c_str(),
      NodeIdToScadaString(source_id).c_str(),
      NodeIdToScadaString(target_id).c_str());

  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected);

  protocol::Request request;
  auto& add_reference = *request.mutable_add_reference();
  ToProto(reference_type_id, *add_reference.mutable_reference_type_id());
  ToProto(source_id, *add_reference.mutable_source_id());
  ToProto(target_id, *add_reference.mutable_target_id());

  sender_->Request(request,
                   [this, callback](const protocol::Response& response) {
                     if (callback)
                       callback(FromProto(response.status()));
                   });
}

void NodeManagementProxy::DeleteReference(
    const scada::NodeId& reference_type_id,
    const scada::NodeId& source_id,
    const scada::NodeId& target_id,
    const scada::StatusCallback& callback) {
  logger().WriteF(LogSeverity::Normal,
                  "DeleteReference request [reference_type_id=%s, "
                  "source_id=%s, target_id=%s]",
                  NodeIdToScadaString(reference_type_id).c_str(),
                  NodeIdToScadaString(source_id).c_str(),
                  NodeIdToScadaString(target_id).c_str());

  if (!sender_)
    return callback(scada::StatusCode::Bad_Disconnected);

  protocol::Request request;
  auto& delete_reference = *request.mutable_delete_reference();
  ToProto(reference_type_id, *delete_reference.mutable_reference_type_id());
  ToProto(source_id, *delete_reference.mutable_source_id());
  ToProto(target_id, *delete_reference.mutable_target_id());

  sender_->Request(request,
                   [this, callback](const protocol::Response& response) {
                     if (callback)
                       callback(FromProto(response.status()));
                   });
}
