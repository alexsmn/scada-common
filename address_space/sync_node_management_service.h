#pragma once

#include "core/node_management_service.h"

class SyncNodeManagementService {
 public:
  virtual ~SyncNodeManagementService() = default;

  virtual std::pair<scada::Status, scada::NodeId> CreateNode(
      const scada::AddNodesItem& input) = 0;
  virtual scada::Status DeleteNode(
      const scada::NodeId& node_id,
      std::vector<scada::NodeId>* dependencies) = 0;

  virtual scada::Status AddReference(const scada::AddReferencesItem& input) = 0;
  virtual scada::Status DeleteReference(
      const scada::DeleteReferencesItem& input) = 0;

  virtual scada::Status ChangeUserPassword(
      const scada::NodeId& user_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password) = 0;
};
