#include "node_service/node_children_fetcher.h"

#include "base/executor.h"
#include "base/strings/strcat.h"
#include "core/attribute_service.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/view_service.h"
#include "model/node_id_util.h"
#include "node_service/node_fetcher.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include "base/debug_util-inl.h"

namespace {

const size_t kMaxFetchChildCount = 100;

std::string NodeIdsToString(
    const std::vector<scada::ReferenceDescription>& references) {
  const std::string& node_ids = boost::algorithm::join(
      references | boost::adaptors::transformed(
                       [](const scada::ReferenceDescription& reference) {
                         return ToString(reference.node_id);
                       }),
      ", ");
  return base::StrCat({"[", node_ids, "]"});
}

void MergeResult(std::map<scada::NodeId, scada::BrowseResult>& results,
                 const scada::NodeId& node_id,
                 scada::BrowseResult&& result) {
  auto [i, ok] = results.try_emplace(node_id, std::move(result));
  if (!ok) {
    scada::BrowseResult& existing_result = i->second;
    existing_result.references.insert(existing_result.references.end(),
                                      result.references.begin(),
                                      result.references.end());
    if (scada::IsBad(existing_result.status_code))
      existing_result.status_code = result.status_code;
  }
}

}  // namespace

NodeChildrenFetcher::NodeChildrenFetcher(NodeChildrenFetcherContext&& context)
    : NodeChildrenFetcherContext{std::move(context)} {}

// static
std::shared_ptr<NodeChildrenFetcher> NodeChildrenFetcher::Create(
    NodeChildrenFetcherContext&& context) {
  return std::shared_ptr<NodeChildrenFetcher>(
      new NodeChildrenFetcher(std::move(context)));
}

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
  LOG_INFO(logger_) << "Browse children completed"
                    << LOG_TAG("DurationMs", duration.InMilliseconds())
                    << LOG_TAG("Status", ToString(status));
  LOG_DEBUG(logger_) << "Browse children completed"
                     << LOG_TAG("DurationMs", duration.InMilliseconds())
                     << LOG_TAG("Status", ToString(status))
                     << LOG_TAG("Inputs", ToString(descriptions))
                     << LOG_TAG("Results", ToString(results));

  assert(!descriptions.empty());

  std::map<scada::NodeId, scada::BrowseResult> merged_results;

  for (size_t i = 0; i < descriptions.size(); ++i) {
    auto& description = descriptions[i];

    if (!status) {
      MergeResult(merged_results, description.node_id,
                  scada::BrowseResult{status.code()});
      continue;
    }

    auto& result = results[i];
    MergeResult(merged_results, description.node_id, std::move(result));
  }

  // TODO: Bulk update.
  for (auto& [node_id, result] : merged_results)
    reference_validator_(node_id, std::move(result));

  assert(children_request_count_ > 0);
  --children_request_count_;
  LOG_INFO(logger_) << "Running requests"
                    << LOG_TAG("Count", children_request_count_);

  FetchPendingNodes();
}

void NodeChildrenFetcher::Fetch(const scada::NodeId& node_id) {
  // Can't be checked when node if refreshed.
  // if (node_availability_handler_(node_id))
  //   return;

  if (!pending_children_set_.emplace(node_id).second)
    return;

  // LOG_INFO(logger_) << "Schedule browse children"
  //                   << LOG_TAG("NodeId", ToString(node_id));

  pending_children_.emplace_back(node_id);

  FetchPendingNodes();
}

void NodeChildrenFetcher::FetchChildren(
    const std::vector<scada::NodeId>& node_ids) {
  assert(!node_ids.empty());

  LOG_INFO(logger_) << "Browse nodes children"
                    << LOG_TAG("NodeIds", ToString(node_ids));

  const auto start_ticks = base::TimeTicks::Now();
  ++children_request_count_;

  std::vector<scada::BrowseDescription> descriptions;
  descriptions.reserve(node_ids.size());
  for (auto& node_id : node_ids) {
    descriptions.push_back(
        {node_id, scada::BrowseDirection::Forward, scada::id::Organizes, true});
    descriptions.push_back({node_id, scada::BrowseDirection::Forward,
                            scada::id::HasSubtype, true});
  }

  view_service_.Browse(
      descriptions,
      BindExecutor(
          executor_,
          [weak_ptr = weak_from_this(), start_ticks, descriptions](
              scada::Status status, std::vector<scada::BrowseResult> results) {
            if (auto ptr = weak_ptr.lock()) {
              ptr->OnBrowseChildrenResult(start_ticks, std::move(status),
                                          std::move(descriptions),
                                          std::move(results));
            }
          }));
}

void NodeChildrenFetcher::Cancel(const scada::NodeId& node_id) {
  if (!pending_children_set_.erase(node_id))
    return;

  auto i =
      std::find(pending_children_.begin(), pending_children_.end(), node_id);
  assert(i != pending_children_.end());
  pending_children_.erase(i);
}

size_t NodeChildrenFetcher::GetPendingNodeCount() const {
  return pending_children_.size() + children_request_count_;
}
