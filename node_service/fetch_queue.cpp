#include "node_service/fetch_queue.h"

#include <algorithm>

void FetchQueue::push(FetchingNode& node) {
  assert(thread_checker_.CalledOnValidThread());

  if (node.pending) {
    assert(find(node) != queue_.end());
    return;
  }

  assert(find(node) == queue_.end());

  node.pending = true;

  queue_.emplace_back(Node{next_sequence_++, &node});
  std::push_heap(queue_.begin(), queue_.end());
}

void FetchQueue::pop() {
  assert(thread_checker_.CalledOnValidThread());

  auto& node = *queue_.front().node;
  assert(node.pending);
  node.pending = false;
  std::pop_heap(queue_.begin(), queue_.end());
  queue_.pop_back();
}

void FetchQueue::erase(FetchingNode& node) {
  assert(thread_checker_.CalledOnValidThread());

  if (!node.pending) {
    assert(find(node) == queue_.end());
    return;
  }

  auto i = find(node);
  assert(i != queue_.end());

  if (i != std::prev(queue_.end()))
    std::iter_swap(i, std::prev(queue_.end()));
  queue_.erase(std::prev(queue_.end()));

  std::make_heap(queue_.begin(), queue_.end());

  node.pending = false;
}

std::vector<FetchQueue::Node>::iterator FetchQueue::find(
    const FetchingNode& node) {
  assert(thread_checker_.CalledOnValidThread());
  return std::find_if(queue_.begin(), queue_.end(),
                      [&node](const Node& n) { return n.node == &node; });
}

std::vector<FetchQueue::Node>::const_iterator FetchQueue::find(
    const FetchingNode& node) const {
  assert(thread_checker_.CalledOnValidThread());
  return std::find_if(queue_.cbegin(), queue_.cend(),
                      [&node](const Node& n) { return n.node == &node; });
}

std::size_t FetchQueue::count(const FetchingNode& node) const {
  assert(thread_checker_.CalledOnValidThread());
  assert(node.pending == (find(node) != queue_.cend()));
  return node.pending ? 1 : 0;
}
