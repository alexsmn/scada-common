#pragma once

#include "core/node_management_service.h"

class SyncNodeManagementService {
 public:
  virtual ~SyncNodeManagementService() = default;

  virtual std::pair<scada::Status, scada::NodeId> CreateNode(
      scada::NodeClass node_class,
      const scada::NodeId& requested_id,
      const scada::NodeId& parent_id,
      const scada::NodeId& type_id,
      scada::NodeAttributes attributes) = 0;
  virtual scada::Status DeleteNode(
      const scada::NodeId& node_id,
      std::vector<scada::NodeId>* dependencies) = 0;

  virtual scada::Status AddReference(const scada::NodeId& reference_type_id,
                                     const scada::NodeId& source_id,
                                     const scada::NodeId& target_id) = 0;
  virtual scada::Status DeleteReference(const scada::NodeId& reference_type_id,
                                        const scada::NodeId& source_id,
                                        const scada::NodeId& target_id) = 0;

  virtual scada::Status ChangeUserPassword(
      const scada::NodeId& user_id,
      const scada::LocalizedText& current_password,
      const scada::LocalizedText& new_password) = 0;
};
