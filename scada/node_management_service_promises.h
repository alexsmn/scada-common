#pragma once

#include "base/awaitable_promise.h"
#include "base/executor_conversions.h"
#include "scada/status_exception.h"
#include "scada/node_management_service.h"

namespace scada {

inline Awaitable<std::vector<AddNodesResult>> AddNodesCompatAsync(
    NodeManagementService& service,
    std::vector<AddNodesItem> inputs) {
  auto results = co_await service.AddNodes(std::move(inputs));
  if (!results.ok()) {
    throw status_exception{results.status()};
  }
  co_return std::move(*results);
}

inline promise<std::vector<AddNodesResult>> AddNode(
    NodeManagementService& service,
    const std::vector<AddNodesItem>& inputs) {
  return ToPromise(MakeThreadAnyExecutor(),
                   AddNodesCompatAsync(service,
                                       std::vector<AddNodesItem>{inputs}));
}

inline Awaitable<NodeId> AddNodeCompatAsync(NodeManagementService& service,
                                            AddNodesItem input) {
  auto result = co_await service.AddNodes({std::move(input)});
  if (!result.ok()) {
    throw status_exception{result.status()};
  }

  assert(result->size() == 1);

  auto& [status_code, node_id] = (*result)[0];
  if (IsBad(status_code)) {
    throw status_exception{status_code};
  }

  co_return std::move(node_id);
}

inline promise<NodeId> AddNode(NodeManagementService& service,
                               const AddNodesItem& input) {
  return ToPromise(MakeThreadAnyExecutor(), AddNodeCompatAsync(service, input));
}

inline Awaitable<void> DeleteNodeCompatAsync(NodeManagementService& service,
                                             DeleteNodesItem input) {
  auto result = co_await service.DeleteNodes({std::move(input)});
  if (!result.ok()) {
    throw status_exception{result.status()};
  }

  assert(result->size() == 1);

  const auto status_code = (*result)[0];
  if (IsBad(status_code)) {
    throw status_exception{status_code};
  }

  co_return;
}

inline promise<void> DeleteNode(NodeManagementService& service,
                                const DeleteNodesItem& input) {
  return ToPromise(MakeThreadAnyExecutor(),
                   DeleteNodeCompatAsync(service, input));
}

}  // namespace scada
