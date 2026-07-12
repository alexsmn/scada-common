#pragma once

#include "common/node_state.h"
#include "node_service/v3/node_fetcher.h"
#include "scada/service_context.h"

namespace scada {
class AttributeService;
class ViewService;
}  // namespace scada

namespace v3 {

// Services a ServiceNodeFetcher fetches from.
struct ServiceNodeFetcherContext {
  scada::ViewService& view_service_;
  scada::AttributeService& attribute_service_;
  scada::ServiceContext service_context_;
};

// Production v3::NodeFetcher: loads node data straight from the OPC UA
// attribute and view services.
//
// Unlike v2's NodeFetcherImpl (which drives a shared FetchingNodeGraph and
// fans out child fetches itself), this fetcher answers one node or one node's
// children per call as a straight-line coroutine and returns the assembled
// NodeState / ReferenceDescriptions to the service, which owns residency and
// schedules follow-up fetches. It reproduces v2's attribute and reference
// selection so nodes come back populated identically.
class ServiceNodeFetcher : private ServiceNodeFetcherContext,
                           public NodeFetcher {
 public:
  explicit ServiceNodeFetcher(ServiceNodeFetcherContext&& context);

  // NodeFetcher
  Awaitable<scada::StatusOr<scada::NodeState>> FetchNode(
      const scada::NodeId& node_id) override;
  Awaitable<scada::StatusOr<scada::ReferenceDescriptions>> FetchChildren(
      const scada::NodeId& node_id) override;
};

}  // namespace v3
