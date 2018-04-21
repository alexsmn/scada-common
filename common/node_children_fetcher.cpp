#include "common/node_children_fetcher.h"

#include "base/logger.h"
#include "common/node_fetcher.h"
#include "common/node_id_util.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"

namespace {
const size_t kMaxFetchChildCount = 100;
}

NodeChildrenFetcher::NodeChildrenFetcher(NodeChildrenFetcherContext&& context)
    : NodeChildrenFetcherContext{std::move(context)} {}

void NodeChildrenFetcher::FetchPendingNodes() {
  if (children_request_count_ == 0 && !pending_children_.empty()) {
    std::vector<scada::NodeId> node_ids;
    node_ids.reserve(std::min(kMaxFetchChildCount, pending_children_.size()));

    while (!pending_children_.empty() &&
           node_ids.size() < kMaxFetchChildCount) {
      auto node_id = std::move(pending_children_.front());
      pending_children_.pop_front();
      pending_children_set_.erase(node_id);
      node_ids.emplace_back(std::move(node_id));
    }

    FetchChildren(std::move(node_ids));
    return;
  }
}

void NodeChildrenFetcher::OnBrowseChildrenResult(
    base::TimeTicks start_ticks,
    scada::Status&& status,
    const std::vector<scada::BrowseDescription>& descriptions,
    std::vector<scada::BrowseResult>&& results) {
  const auto duration = base::TimeTicks::Now() - start_ticks;
  logger_.WriteF(LogSeverity::Normal,
                 "Browse children completed in %u ms with status: %ls",
                 static_cast<unsigned>(duration.InMilliseconds()),
                 ToString16(status).c_str());

  assert(!descriptions.empty());

  std::map<scada::NodeId, ReferenceMap> references;

  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];

    // WARNING: Map elements must be inserted even if status is bad.
    auto& target_ids =
        references[description.node_id][description.reference_type_id];

    if (!status)
      continue;

    assert(descriptions.size() == results.size());
    auto& result = results[i];
    for (auto& reference : result.references) {
      assert(reference.forward);
      target_ids.emplace(std::move(reference.node_id),
                         std::move(reference.reference_type_id));
    }
  }

  assert(children_request_count_ > 0);
  --children_request_count_;
  logger_.WriteF(LogSeverity::Normal, "%Iu children requests are running",
                 children_request_count_);

  FetchPendingNodes();

  for (auto& [node_id, references] : references)
    reference_validator_(node_id, std::move(references));
}

void NodeChildrenFetcher::Fetch(const scada::NodeId& node_id) {
  // Can't be checked when node if refreshed.
  // if (node_availability_handler_(node_id))
  //   return;

  if (!pending_children_set_.emplace(node_id).second)
    return;

  logger_.WriteF(LogSeverity::Normal, "Schedule browse children of node %s",
                 node_id.ToString().c_str());

  pending_children_.emplace_back(node_id);

  FetchPendingNodes();
}

void NodeChildrenFetcher::FetchChildren(
    const std::vector<scada::NodeId>& node_ids) {
  assert(!node_ids.empty());

  logger_.WriteF(LogSeverity::Normal, "Browse children (%Iu)", node_ids.size());

  const auto start_ticks = base::TimeTicks::Now();
  ++children_request_count_;

  std::vector<scada::BrowseDescription> descriptions;
  descriptions.reserve(node_ids.size() * 2);
  for (auto& node_id : node_ids) {
    descriptions.push_back(
        {node_id, scada::BrowseDirection::Forward, scada::id::Organizes, true});
    descriptions.push_back({node_id, scada::BrowseDirection::Forward,
                            scada::id::HasSubtype, true});
  }

  view_service_.Browse(
      descriptions,
      [weak_ptr = weak_factory_.GetWeakPtr(), start_ticks, descriptions](
          scada::Status&& status, std::vector<scada::BrowseResult>&& results) {
        if (auto* ptr = weak_ptr.get()) {
          ptr->OnBrowseChildrenResult(start_ticks, std::move(status),
                                      std::move(descriptions),
                                      std::move(results));
        }
      });
}

void NodeChildrenFetcher::Cancel(const scada::NodeId& node_id) {
  if (!pending_children_set_.erase(node_id))
    return;

  auto i =
      std::find(pending_children_.begin(), pending_children_.end(), node_id);
  assert(i != pending_children_.end());
  pending_children_.erase(i);
}
