#pragma once

#include "base/memory/weak_ptr.h"
#include "core/node_id.h"

#include <memory>
#include <vector>

namespace protocol {
class Reference;
class Request;
}

namespace scada {
class NodeAttributes;
class NodeManagementService;
enum class NodeClass;
}

class Logger;
class MessageSender;

class NodeManagementStub {
 public:
  NodeManagementStub(MessageSender& sender,
                     scada::NodeManagementService& service,
                     const scada::NodeId& user_id,
                     std::shared_ptr<Logger> logger);

  void OnRequestReceived(const protocol::Request& request);

 private:
  void OnCreateNode(unsigned request_id, const scada::NodeId& requested_id,
                    const scada::NodeId& parent_id, scada::NodeClass node_class,
                    const scada::NodeId& type_id, scada::NodeAttributes attributes);
  void OnModifyNodes(unsigned request_id, const std::vector<std::pair<scada::NodeId, scada::NodeAttributes>>& attributes);
  void OnDeleteNode(unsigned request_id, const scada::NodeId& id, bool return_relations);
  void OnChangeUserPassword(unsigned request_id, const scada::NodeId& user_node_id,
                            const std::string& current_pass,
                            const std::string& new_pass);
  void OnAddReference(unsigned request_id, const protocol::Reference& request);
  void OnDeleteReference(unsigned request_id, const protocol::Reference& request);

  MessageSender& sender_;

  scada::NodeManagementService& service_;

  // Identifier of logged user for access control.
  scada::NodeId user_id_;

  std::shared_ptr<Logger> logger_;

  base::WeakPtrFactory<NodeManagementStub> weak_factory_;
};
