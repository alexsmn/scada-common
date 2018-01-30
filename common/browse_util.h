#pragma once

#include "common/node_service.h"
#include "core/configuration_types.h"
#include "core/view_service.h"

#include <algorithm>

// Callback = void(const scada::Status& status, const
// scada::ReferenceDescription& reference)
template <class Callback>
inline void BrowseReference(scada::ViewService& view_service,
                            const scada::BrowseDescription& description,
                            const Callback& callback) {
  view_service.Browse(
      {description},
      [callback](const scada::Status& status,
                 const std::vector<scada::BrowseResult>& results) {
        if (!status) {
          assert(results.empty());
          callback(status, {});
          return;
        }

        assert(results.size() == 1);
        auto& result = results.front();

        if (result.references.empty()) {
          assert(result.references.empty());
          callback(scada::StatusCode::Bad_WrongReferenceId, {});
        } else {
          // FIXME:
          // assert(references.size() == 1);
          callback(status, result.references.front());
        }
      });
}

// Callback = void(const scada::Status& status, const NodeRef& node)
template <class Callback>
inline void BrowseNode(scada::ViewService& view_service,
                       NodeService& node_service,
                       const scada::BrowseDescription& description,
                       const Callback& callback) {
  BrowseReference(
      view_service, description,
      [&node_service, callback](const scada::Status& status,
                                const scada::ReferenceDescription& reference) {
        if (!status) {
          callback(status, nullptr);
          return;
        }

        node_service.GetNode(reference.node_id)
            .Fetch([callback](const NodeRef& node) {
              callback(node.status(), node);
            });
      });
}

// Callback = void(const scada::NodeId& parent_id)
template <class Callback>
inline void BrowseParentId(scada::ViewService& view_service,
                           const scada::NodeId& node_id,
                           const scada::NodeId& reference_type_id,
                           const Callback& callback) {
  BrowseReference(
      view_service,
      {node_id, scada::BrowseDirection::Inverse, reference_type_id, true},
      [callback](const scada::Status& status,
                 const scada::ReferenceDescription& reference) {
        callback(reference.node_id);
      });
}

// Callback = void(const scada::Status& status, const NodeRef& node)
template <class Callback>
inline void BrowseParent(scada::ViewService& view_service,
                         NodeService& node_service,
                         const scada::NodeId& node_id,
                         const scada::NodeId& reference_type_id,
                         const Callback& callback) {
  BrowseNode(
      view_service, node_service,
      {node_id, scada::BrowseDirection::Inverse, reference_type_id, true},
      callback);
}

// Callback = void(std::vector<scada::Status> statuses, std::vector<NodeRef>
// nodes)
template <class Callback>
inline void RequestNodes(NodeService& service,
                         const std::vector<scada::NodeId>& node_ids,
                         const Callback& callback) {
  struct Result {
    Result(size_t count, const Callback& callback)
        : statuses{count, scada::StatusCode::Bad},
          nodes{count},
          callback{callback} {}

    ~Result() { callback(std::move(statuses), std::move(nodes)); }

    std::vector<scada::Status> statuses;
    std::vector<NodeRef> nodes;
    const Callback callback;
  };

  auto result = std::make_shared<Result>(node_ids.size(), callback);
  for (size_t i = 0; i < node_ids.size(); ++i) {
    service.GetNode(node_ids[i]).Fetch([result, i](const NodeRef& node) {
      result->statuses[i] = node.status();
      result->nodes[i] = node;
    });
  }
}

// Callback = void(const scada::Status& status, std::vector<NodeRef> nodes)
template <class Callback>
inline void BrowseNodes(scada::ViewService& view_service,
                        NodeService& node_service,
                        const scada::BrowseDescription& description,
                        const Callback& callback) {
  view_service.Browse(
      {description}, [&node_service, callback](
                         const scada::Status& status,
                         const std::vector<scada::BrowseResult>& results) {
        if (!status) {
          callback(status, {});
          return;
        }

        assert(results.size() == 1);
        auto& result = results.front();

        std::vector<scada::NodeId> node_ids;
        node_ids.reserve(result.references.size());
        for (auto& ref : result.references)
          node_ids.emplace_back(ref.node_id);

        RequestNodes(node_service, node_ids,
                     [callback](const std::vector<scada::Status>&,
                                std::vector<NodeRef> nodes) {
                       nodes.erase(
                           std::remove(nodes.begin(), nodes.end(), nullptr),
                           nodes.end());
                       callback(scada::StatusCode::Good, std::move(nodes));
                     });
      });
}
