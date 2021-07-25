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

  virtual std::vector<scada::StatusCode> AddReferences(
      base::span<const scada::AddReferencesItem> inputs) = 0;
  virtual std::vector<scada::StatusCode> DeleteReferences(
      base::span<const scada::DeleteReferencesItem> inputs) = 0;

  virtual scada::Status ChangeUserPassword(
      const scada::NodeId& user_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password) = 0;
};
