#pragma once

#include "base/observer_list.h"
#include "common/node_service.h"
#include "core/view_service.h"

#include <map>

namespace scada {
class AttributeService;
struct ModelChangeEvent;
}  // namespace scada

class Logger;
class RemoteNodeModel;
struct NodeModelImplReference;

struct RemoteNodeServiceContext {
  const std::shared_ptr<Logger> logger_;
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
};

class RemoteNodeService : private RemoteNodeServiceContext,
                          private scada::ViewEvents,
                          public NodeService {
 public:
  explicit RemoteNodeService(RemoteNodeServiceContext&& context);
  ~RemoteNodeService();

  // NodeService
  virtual NodeRef GetNode(const scada::NodeId& node_id) override;
  virtual void Subscribe(NodeRefObserver& observer) const override;
  virtual void Unsubscribe(NodeRefObserver& observer) const override;

 private:
  using BrowseCallback =
      std::function<void(const scada::Status& status,
                         scada::ReferenceDescriptions references)>;
  void Browse(const scada::BrowseDescription& description,
              const BrowseCallback& callback);

  void AddNodeObserver(const scada::NodeId& node_id, NodeRefObserver& observer);
  void RemoveNodeObserver(const scada::NodeId& node_id,
                          NodeRefObserver& observer);

  // Returns fetched or unfetched node impl.
  std::shared_ptr<RemoteNodeModel> GetNodeImpl(
      const scada::NodeId& node_id,
      const scada::NodeId& depended_id);

  void CompletePartialNode(const std::shared_ptr<RemoteNodeModel>& node);

  using Observers = base::ObserverList<NodeRefObserver>;

  const Observers* GetNodeObservers(const scada::NodeId& node_id) const;

  // scada::ViewService
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;
  virtual void OnNodeSemanticsChanged(const scada::NodeId& node_id) override;

  mutable Observers observers_;
  std::map<scada::NodeId, Observers> node_observers_;

  std::map<scada::NodeId, std::shared_ptr<RemoteNodeModel>> nodes_;

  friend class RemoteNodeModel;
};
