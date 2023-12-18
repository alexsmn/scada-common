#pragma once

#include "scada/node_management_service.h"

#include <span>

class SyncNodeManagementService {
 public:
  virtual ~SyncNodeManagementService() = default;

  virtual std::pair<scada::StatusCode, scada::NodeId> CreateNode(
      const scada::AddNodesItem& input) = 0;
  virtual scada::StatusCode DeleteNode(
      const scada::NodeId& node_id,
      std::vector<scada::NodeId>* dependencies) = 0;

  virtual std::vector<scada::StatusCode> AddReferences(
      std::span<const scada::AddReferencesItem> inputs) = 0;
  virtual std::vector<scada::StatusCode> DeleteReferences(
      std::span<const scada::DeleteReferencesItem> inputs) = 0;
};
