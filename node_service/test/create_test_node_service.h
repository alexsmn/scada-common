#pragma once

#include "address_space/address_space_util.h"
#include "node_service/static/static_node_service.h"

#include <memory>

class NodeService;

namespace node_service::test {

// Builds a version-agnostic, synchronous NodeService over a pre-populated
// address space, for tests that only need a working node graph to read and
// navigate (display names, parents, references). Backed by StaticNodeService,
// which answers reads/navigation directly from the materialized NodeStates
// with no asynchronous fetch, so reads are available immediately.
//
// Replaces the former v1::CreateTestNodeService, which wrapped the deleted v1
// NodeService implementation.
inline std::shared_ptr<NodeService> CreateTestNodeService(
    const scada::AddressSpace& address_space) {
  auto node_service = std::make_shared<StaticNodeService>();
  node_service->AddAll(scada::MakeNodeStates(address_space));
  return node_service;
}

}  // namespace node_service::test
