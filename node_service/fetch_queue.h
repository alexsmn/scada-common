#pragma once

#include "base/threading/thread_checker.h"
#include "node_service/fetching_node.h"

#include <vector>

class FetchQueue {
 public:
  bool empty() const { return queue_.empty(); };
  std::size_t size() const { return queue_.size(); }

  FetchingNode& top() { return *queue_.front().node; }

  std::size_t count(const FetchingNode& node) const;

  void push(FetchingNode& node);
  void pop();

  void erase(FetchingNode& node);

 private:
  struct Node {
    auto key() const { return std::tie(node->pending_sequence, sequence); }

    bool operator<(const Node& other) const { return key() > other.key(); }

    unsigned sequence = 0;
    FetchingNode* node = nullptr;
  };

  std::vector<Node>::iterator find(const FetchingNode& node);
  std::vector<Node>::const_iterator find(const FetchingNode& node) const;

  THREAD_CHECKER(thread_checker_);

  std::vector<Node> queue_;
  unsigned next_sequence_ = 0;
};
