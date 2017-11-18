#pragma once

#include "base/observer_list.h"
#include "node_ref_service.h"

#include <map>

namespace scada {
class AttributeService;
}

class Logger;
class NodeModelImpl;
struct NodeModelImplReference;

struct NodeRefServiceImplContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
};

class NodeRefServiceImpl : private NodeRefServiceImplContext,
                           private scada::ViewEvents,
                           public NodeRefService {
 public:
  explicit NodeRefServiceImpl(NodeRefServiceImplContext&& context);
  ~NodeRefServiceImpl();

  // NodeRefService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void AddObserver(NodeRefObserver& observer) override;
  virtual void RemoveObserver(NodeRefObserver& observer) override;

 private:
  using BrowseCallback = std::function<void(const scada::Status& status, scada::ReferenceDescriptions references)>;
  void Browse(const scada::BrowseDescription& description, const BrowseCallback& callback);

  void AddNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer);
  void RemoveNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer);

  // Returns fetched or unfetched node impl.
  std::shared_ptr<NodeModelImpl> GetNodeImpl(const scada::NodeId& node_id, const scada::NodeId& depended_id);

  void CompletePartialNode(const std::shared_ptr<NodeModelImpl>& node);

  using Observers = base::ObserverList<NodeRefObserver>;

  const Observers* GetNodeObservers(const scada::NodeId& node_id) const;

  // scada::ViewService
  virtual void OnNodeAdded(const scada::NodeId& node_id) override;
  virtual void OnNodeDeleted(const scada::NodeId& node_id) override;
  virtual void OnReferenceAdded(const scada::NodeId& node_id) ;
  virtual void OnReferenceDeleted(const scada::NodeId& node_id) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  Observers observers_;
  std::map<scada::NodeId, Observers> node_observers_;

  std::map<scada::NodeId, std::shared_ptr<NodeModelImpl>> nodes_;

  friend class NodeModelImpl;
};
