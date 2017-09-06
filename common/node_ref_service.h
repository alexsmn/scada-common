#pragma once

#include <functional>

#include "common/node_ref.h"
#include "core/status.h"
#include "core/view_service.h"

namespace scada {
class AttributeService;
}

class Logger;
class NodeRefObserver;
struct NodeRefImplReference;

class NodeRefService {
 public:
  virtual ~NodeRefService() {}

  virtual NodeRef GetPartialNode(const scada::NodeId& node_id) = 0;

  virtual NodeRef GetCachedNode(const scada::NodeId& node_id) const = 0;

  using RequestNodeCallback = std::function<void(const scada::Status& status, const NodeRef& node)>;
  virtual void RequestNode(const scada::NodeId& node_id, const RequestNodeCallback& callback) = 0;

  using BrowseCallback = std::function<void(const scada::Status& status, scada::ReferenceDescriptions references)>;
  virtual void Browse(const scada::BrowseDescription& description, const BrowseCallback& callback) = 0;

  virtual void AddObserver(NodeRefObserver& observer) = 0;
  virtual void RemoveObserver(NodeRefObserver& observer) = 0;

  virtual void AddNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer) = 0;
  virtual void RemoveNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer) = 0;
};
