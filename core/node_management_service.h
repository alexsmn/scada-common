#pragma once

#include <functional>
#include <set>

#include "core/status.h"
#include "core/configuration_types.h"

namespace scada {

enum class NodeClass;

typedef std::set<NodeId> NodeIdSet;

class NodeManagementService {
 public:
  virtual ~NodeManagementService() {}

  typedef std::function<void(const Status& /*status*/,
                             const NodeId& /*node_id*/)> CreateNodeCallback;
  virtual void CreateNode(const NodeId& requested_id, const NodeId& parent_id,
                          NodeClass node_class, const NodeId& type_id, NodeAttributes attributes,
                          const CreateNodeCallback& callback) = 0;

  typedef std::function<void(const Status&)> StatusCallback;
  typedef std::function<void(const std::vector<Status>&)> MultiStatusCallback;
  virtual void ModifyNodes(const std::vector<std::pair<NodeId, NodeAttributes>>& attributes,
                           const MultiStatusCallback& callback) = 0;

  // Delete record from table. If |return_relations| is true and deletion fails,
  // it gets list of related records, which must be deleted before.
  typedef std::function<void(const Status& /*status*/,
                             const NodeIdSet* /*references*/)> DeleteNodeCallback;
  virtual void DeleteNode(const NodeId& node_id, bool return_relations,
                          const DeleteNodeCallback& callback) = 0;

  virtual void ChangeUserPassword(const NodeId& user_node_id,
                                  const std::string& current_password,
                                  const std::string& new_password,
                                  const StatusCallback& callback) = 0;

  virtual void AddReference(const NodeId& reference_type_id, const NodeId& source_id, const NodeId& target_id,
                            const StatusCallback& callback) = 0;
  virtual void DeleteReference(const NodeId& reference_type_id, const NodeId& source_id, const NodeId& target_id,
                               const StatusCallback& callback) = 0;
};

} // namespace scada