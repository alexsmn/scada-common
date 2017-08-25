#pragma once

#include <map>

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "common/node_ref.h"

class NodeRefService;

// TODO: Process NodeRefObserver events.
class NodeFetcher {
 public:
  explicit NodeFetcher(NodeRefService& node_service) : node_service_{node_service} {}

  NodeRef FetchNode(const scada::NodeId& node_id);

  class Observer {
   public:
    virtual void OnNodeFetched(const NodeRef& node) = 0;
  };

  void AddObserver(Observer& observer) { observers_.AddObserver(&observer); }
  void RemoveObserver(Observer& observer)  { observers_.RemoveObserver(&observer); }

 private:
  NodeRefService& node_service_;

  std::map<scada::NodeId, NodeRef> fetched_nodes_;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<NodeFetcher> weak_ptr_factory_{this};
};

std::string FetchNodeName(NodeFetcher& fetcher, const scada::NodeId& node_id);
base::string16 FetchNodeTitle(NodeFetcher& fetcher, const scada::NodeId& node_id);
