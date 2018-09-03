#pragma once

#include <memory>

#include "core/node_management_service.h"

class Logger;
class MessageSender;

class NodeManagementProxy : public scada::NodeManagementService {
 public:
  explicit NodeManagementProxy(std::shared_ptr<Logger> logger);

  void OnChannelOpened(MessageSender& sender);
  void OnChannelClosed();

  // scada::NodeManagementService
  virtual void CreateNode(const scada::NodeId& requested_id,
                          const scada::NodeId& parent_id,
                          scada::NodeClass node_class,
                          const scada::NodeId& type_id,
                          scada::NodeAttributes attributes,
                          const CreateNodeCallback& callback) override;
  virtual void ModifyNodes(
      const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>&
          attributes,
      const scada::ModifyNodesCallback& callback) override;
  virtual void DeleteNode(const scada::NodeId& node_id,
                          bool return_dependencies,
                          const scada::DeleteNodeCallback& callback) override;
  virtual void ChangeUserPassword(
      const scada::NodeId& user_node_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password,
      const scada::StatusCallback& callback) override;
  virtual void AddReference(const scada::NodeId& reference_type_id,
                            const scada::NodeId& source_id,
                            const scada::NodeId& target_id,
                            const scada::StatusCallback& callback) override;
  virtual void DeleteReference(const scada::NodeId& reference_type_id,
                               const scada::NodeId& source_id,
                               const scada::NodeId& target_id,
                               const scada::StatusCallback& callback) override;

 private:
  Logger& logger() { return *logger_; }

  std::shared_ptr<Logger> logger_;

  MessageSender* sender_ = nullptr;
};
