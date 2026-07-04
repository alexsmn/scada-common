#include "node_service/fetch_queue.h"

#include <algorithm>

#include "base/check.h"

void FetchQueue::push(FetchingNode& node) {
  thread_checker_.CheckCalledOnValidThread();

  if (node.pending) {
    base::Check(find(node) != queue_.end(), "pending node missing from queue");
    return;
  }

  base::Check(find(node) == queue_.end(), "non-pending node already queued");

  node.pending = true;

  queue_.emplace_back(Node{next_sequence_++, &node});
  std::push_heap(queue_.begin(), queue_.end());
}

void FetchQueue::pop() {
  thread_checker_.CheckCalledOnValidThread();

  auto& node = *queue_.front().node;
  base::Check(node.pending, "popped node is not pending");
  node.pending = false;
  std::pop_heap(queue_.begin(), queue_.end());
  queue_.pop_back();
}

void FetchQueue::erase(FetchingNode& node) {
  thread_checker_.CheckCalledOnValidThread();

  if (!node.pending) {
    base::Check(find(node) == queue_.end(),
                "non-pending node present in queue");
    return;
  }

  auto i = find(node);
  base::Check(i != queue_.end(), "pending node missing from queue");

  if (i != std::prev(queue_.end()))
    std::iter_swap(i, std::prev(queue_.end()));
  queue_.erase(std::prev(queue_.end()));

  std::make_heap(queue_.begin(), queue_.end());

  node.pending = false;
}

std::vector<FetchQueue::Node>::iterator FetchQueue::find(
    const FetchingNode& node) {
  thread_checker_.CheckCalledOnValidThread();
  return std::find_if(queue_.begin(), queue_.end(),
                      [&node](const Node& n) { return n.node == &node; });
}

std::vector<FetchQueue::Node>::const_iterator FetchQueue::find(
    const FetchingNode& node) const {
  thread_checker_.CheckCalledOnValidThread();
  return std::find_if(queue_.cbegin(), queue_.cend(),
                      [&node](const Node& n) { return n.node == &node; });
}

std::size_t FetchQueue::count(const FetchingNode& node) const {
  thread_checker_.CheckCalledOnValidThread();
  base::Check(node.pending == (find(node) != queue_.cend()),
              "pending flag inconsistent with queue contents");
  return node.pending ? 1 : 0;
}
