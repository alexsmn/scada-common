#pragma once

#include <functional>
#include <vector>

#include "core/configuration_types.h"
#include "core/node_attributes.h"
#include "core/status.h"

namespace scada {

enum class NodeClass;

using StatusCallback = std::function<void(Status&&)>;
using ModifyNodesCallback =
    std::function<void(Status&&, std::vector<Status>&&)>;
using DeleteNodeCallback =
    std::function<void(Status&& status,
                       std::vector<scada::NodeId>&& dependencies)>;

class NodeManagementService {
 public:
  virtual ~NodeManagementService() {}

  typedef std::function<void(Status&& status, const NodeId& node_id)>
      CreateNodeCallback;
  virtual void CreateNode(const NodeId& requested_id,
                          const NodeId& parent_id,
                          NodeClass node_class,
                          const NodeId& type_id,
                          NodeAttributes attributes,
                          const CreateNodeCallback& callback) = 0;

  virtual void ModifyNodes(
      const std::vector<std::pair<NodeId, NodeAttributes>>& nodes,
      const ModifyNodesCallback& callback) = 0;

  // Delete record from table. If |return_dependencies| is true and deletion
  // fails, it gets list of related records, which must be deleted before.
  virtual void DeleteNode(const NodeId& node_id,
                          bool return_dependencies,
                          const DeleteNodeCallback& callback) = 0;

  virtual void ChangeUserPassword(const NodeId& user_node_id,
                                  const LocalizedText& current_password,
                                  const LocalizedText& new_password,
                                  const StatusCallback& callback) = 0;

  virtual void AddReference(const NodeId& reference_type_id,
                            const NodeId& source_id,
                            const NodeId& target_id,
                            const StatusCallback& callback) = 0;
  virtual void DeleteReference(const NodeId& reference_type_id,
                               const NodeId& source_id,
                               const NodeId& target_id,
                               const StatusCallback& callback) = 0;
};

}  // namespace scada
