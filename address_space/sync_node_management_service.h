#pragma once

#include "core/node_management_service.h"

class SyncNodeManagementService {
 public:
  virtual ~SyncNodeManagementService() = default;

  virtual std::pair<scada::StatusCode, scada::NodeId> CreateNode(
      const scada::AddNodesItem& input) = 0;
  virtual scada::StatusCode DeleteNode(
      const scada::NodeId& node_id,
      std::vector<scada::NodeId>* dependencies) = 0;

  virtual scada::StatusCode AddReference(
      const scada::AddReferencesItem& input) = 0;
  virtual scada::StatusCode DeleteReference(
      const scada::DeleteReferencesItem& input) = 0;

  virtual scada::Status ChangeUserPassword(
      const scada::NodeId& user_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password) = 0;
};
