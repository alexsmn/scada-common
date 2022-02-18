#pragma once

#include "base/strings/string16.h"
#include "node_service/node_ref.h"

#include <memory>
#include <queue>

class NodeService;

bool IsSubtypeOf(NodeRef type_definition, const scada::NodeId& supertype_id);

bool IsInstanceOf(const NodeRef& node, const scada::NodeId& type_definition_id);

bool CanCreate(const NodeRef& parent, const NodeRef& component_type_definition);

std::vector<NodeRef> GetDataVariables(const NodeRef& node);

std::u16string GetFullDisplayName(const NodeRef& node);

scada::LocalizedText GetDisplayName(NodeService& node_service,
                                    const scada::NodeId& node_id);

template <class Callback>
struct TreeFetcher
    : public std::enable_shared_from_this<TreeFetcher<Callback>> {
  TreeFetcher(Callback&& callback) : callback{std::move(callback)} {}

  void Start(const NodeRef& root) {
    queue.emplace(root);
    Run();
  }

  void Run() {
    if (queue.empty()) {
      callback();
      return;
    }

    auto node = std::move(queue.front());
    queue.pop();

    node.Fetch(NodeFetchStatus::NodeAndChildren(),
               [this, ref = this->shared_from_this()](const NodeRef& node) {
                 for (const auto& child : node.targets(scada::id::Organizes))
                   queue.emplace(child);
                 Run();
               });
  }

  const Callback callback;
  std::queue<NodeRef> queue;
};

template <class Callback>
void FetchTree(const NodeRef& root, Callback&& callback) {
  std::make_shared<TreeFetcher<Callback>>(std::move(callback))->Start(root);
}
