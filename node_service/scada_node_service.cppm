// scada.node_service — named C++20 module facade over the top-level
// common/node_service headers (the v1/v2/v3/proxy implementation targets
// stay header-based).
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md). `export import scada.common;` mirrors
// node_service's PUBLIC link.
//
// Not exported (include the header textually):
//  - the global free operators and helpers for NodeFetchStatus
//    (node_fetch_status.h): the global-namespace operator set (|, &, |=, <<)
//    plus the Includes()/IsEmpty() predicates are never exported
//    (exporting global operator sets would drag every visible global
//    overload, incl. Boost's). Consumers include the header textually and
//    reach the enum type via the `using ::NodeFetchStatus;` re-export below;
//  - node_service::internal (node_service_factory_services.h).

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "node_service/base_node_model.h"
#include "node_service/cached_node_service.h"
#include "node_service/fetch_queue.h"
#include "node_service/fetching_node.h"
#include "node_service/fetching_node_graph.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_children_fetcher.h"
#include "node_service/node_events.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_fetcher.h"
#include "node_service/node_fetcher_impl.h"
#include "node_service/node_format.h"
#include "node_service/node_model.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_service_factory.h"
#include "node_service/node_util.h"

export module scada.node_service;

export import scada.common;

// Keep the GMF-attached std::hash<NodeRef> specialization (node_ref.h)
// decl-reachable for import-only TUs.
static_assert(sizeof(std::hash<NodeRef>) > 0);

export {
  // base_node_model.h / node_model.h / cached_node_service.h
  using ::BaseNodeModel;
  using ::cached_node_service;
  using ::NodeModel;

  // fetch_queue.h / fetching_node.h / fetching_node_graph.h
  using ::FetchingNode;
  using ::FetchingNodeGraph;
  using ::FetchQueue;

  // node_awaitable.h / node_util.h
  using ::FetchChildren;
  using ::FetchNode;
  using ::FetchNodeStatus;
  using ::FetchTree;
  using ::GetDataVariables;
  using ::GetDisplayName;
  using ::IsInstanceOf;
  using ::IsSubtypeOf;
  using ::WaitForPendingNodes;

  // node_children_fetcher.h
  using ::NodeChildrenFetcher;
  using ::NodeChildrenFetcherContext;
  using ::ReferenceValidator;

  // node_events.h
  using ::ModelChangedCallback;
  using ::NodeFetchedCallback;
  using ::NodeFetchedEvent;
  using ::NodeSemanticChangedCallback;
  using ::NodeSignals;
  using ::NodeStateChangedCallback;
  using ::NodeStateChangedEvent;

  // node_fetch_status.h / node_fetcher.h / node_fetcher_impl.h
  using ::FetchCompletedHandler;
  using ::FetchCompletedResult;
  using ::NodeFetcher;
  using ::NodeFetcherImpl;
  using ::NodeFetcherImplContext;
  using ::NodeFetchStatus;
  using ::NodeFetchStatuses;
  using ::NodeValidator;

  // node_ref.h / node_service.h / node_service_factory.h
  using ::CoroutineNodeServiceContext;
  using ::CreateNodeService;
  using ::DataServicesNodeServiceContext;
  using ::NodeRef;
  using ::NodeService;
  using ::NodeServiceContext;
}  // export
