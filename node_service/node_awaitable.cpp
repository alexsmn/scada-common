#include "node_service/node_awaitable.h"

#include "node_service/node_observer.h"
#include "node_service/node_service.h"
#include "scada/status_exception.h"

#include <boost/asio/async_result.hpp>
#include <boost/asio/use_awaitable.hpp>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace {

void ThrowIfBad(scada::Status status) {
  if (status.bad()) {
    throw scada::status_exception{std::move(status)};
  }
}

template <class CompletionToken = boost::asio::use_awaitable_t<>>
auto AwaitFetch(const NodeRef& node,
                const NodeFetchStatus& fetch_status,
                CompletionToken&& token = {}) {
  auto initiate = [node, fetch_status]<typename Handler>(
                      Handler&& handler) mutable {
    auto completion =
        std::make_shared<std::decay_t<Handler>>(std::forward<Handler>(handler));

    node.Fetch(fetch_status, [completion](const NodeRef& node) mutable {
      (*completion)(node.status());
    });
  };

  return boost::asio::async_initiate<CompletionToken, void(scada::Status)>(
      initiate, token);
}

class PendingNodesWaiter final : private NodeRefObserver,
                                 public std::enable_shared_from_this<
                                     PendingNodesWaiter> {
 public:
  explicit PendingNodesWaiter(NodeService& node_service)
      : node_service_{node_service} {}

  template <typename Handler>
  void Wait(Handler&& handler) {
    if (node_service_.GetPendingTaskCount() == 0) {
      std::forward<Handler>(handler)();
      return;
    }

    callback_ = [keep_alive = shared_from_this(),
                 completion = std::make_shared<std::decay_t<Handler>>(
                     std::forward<Handler>(handler))]() mutable {
      (*completion)();
    };

    node_service_.Subscribe(*this);
    subscribed_ = true;
    TryResolve();
  }

  ~PendingNodesWaiter() {
    if (subscribed_) {
      node_service_.Unsubscribe(*this);
    }
  }

 private:
  void OnNodeFetched(const NodeFetchedEvent&) override { TryResolve(); }

  void TryResolve() {
    if (resolved_ || node_service_.GetPendingTaskCount() != 0) {
      return;
    }

    resolved_ = true;
    if (subscribed_) {
      node_service_.Unsubscribe(*this);
      subscribed_ = false;
    }

    auto callback = std::move(callback_);
    callback_ = nullptr;
    if (callback) {
      std::move(callback)();
    }
  }

  NodeService& node_service_;
  std::function<void()> callback_;
  bool subscribed_ = false;
  bool resolved_ = false;
};

template <class CompletionToken = boost::asio::use_awaitable_t<>>
auto AwaitPendingNodes(NodeService& node_service, CompletionToken&& token = {}) {
  auto initiate = [&node_service]<typename Handler>(Handler&& handler) mutable {
    auto waiter = std::make_shared<PendingNodesWaiter>(node_service);
    waiter->Wait([waiter, completion = std::make_shared<std::decay_t<Handler>>(
                              std::forward<Handler>(handler))]() mutable {
      (*completion)();
    });
  };

  return boost::asio::async_initiate<CompletionToken, void()>(initiate, token);
}

}  // namespace

Awaitable<void> FetchNode(const NodeRef& node) {
  if (node.fetched()) {
    ThrowIfBad(node.status());
    co_return;
  }

  ThrowIfBad(co_await AwaitFetch(node, NodeFetchStatus::NodeOnly()));
}

Awaitable<void> FetchChildren(const NodeRef& node) {
  if (node.children_fetched()) {
    co_return;
  }

  auto status = co_await AwaitFetch(node, NodeFetchStatus::NodeAndChildren());
  assert(node.fetched());
  assert(node.type_definition().fetched());
  assert(node.children_fetched());
  ThrowIfBad(std::move(status));
}

Awaitable<void> WaitForPendingNodes(NodeService& node_service) {
  co_await AwaitPendingNodes(node_service);
}

Awaitable<void> FetchRecursive(const NodeRef& node,
                               const scada::NodeId& ref_type_id) {
  co_await FetchChildren(node);
  for (const auto& target : node.targets(ref_type_id)) {
    co_await FetchRecursive(target, ref_type_id);
  }
}

Awaitable<void> FetchTypeSystem(NodeService& node_service) {
  co_await FetchRecursive(node_service.GetNode(scada::id::TypesFolder),
                          scada::id::HierarchicalReferences);
}
