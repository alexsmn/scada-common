#pragma once

#include "node_ref_service.h"

// Callback = void(const scada::Status& status, const scada::ReferenceDescription& reference)
template<class Callback>
inline void BrowseReference(NodeRefService& service, const scada::BrowseDescription& description, const Callback& callback) {
  service.Browse(description, [callback](const scada::Status& status, const scada::ReferenceDescriptions& references) {
    if (!status) {
      assert(references.empty());
      callback(status, {});
    } else if (references.empty()) {
      assert(references.empty());
      callback(scada::StatusCode::Bad_WrongReferenceId, {});
    } else {
      // FIXME:
      // assert(references.size() == 1);
      callback(status, references.front());
    }
  });
}

// Callback = void(const scada::Status& status, const NodeRef& node)
template<class Callback>
inline void BrowseNode(NodeRefService& service, const scada::BrowseDescription& description, const Callback& callback) {
  BrowseReference(service, description,
    [&service, callback](const scada::Status& status, const scada::ReferenceDescription& reference) {
      if (!status) {
        callback(status, nullptr);
        return;
      }
      service.GetNode(reference.node_id).Fetch([callback](NodeRef node) {
        callback(node.status(), node);
      });
    });
}

// Callback = void(const scada::NodeId& parent_id)
template<class Callback>
inline void BrowseParentId(NodeRefService& service, const scada::NodeId& node_id, const scada::NodeId& reference_type_id, const Callback& callback) {
  BrowseReference(service, {node_id, scada::BrowseDirection::Inverse, reference_type_id, true},
      [callback](const scada::Status& status, const scada::ReferenceDescription& reference) {
        callback(reference.node_id);
      });
}

// Callback = void(const scada::Status& status, const NodeRef& node)
template<class Callback>
inline void BrowseParent(NodeRefService& service, const scada::NodeId& node_id, const scada::NodeId& reference_type_id, const Callback& callback) {
  BrowseNode(service, {node_id, scada::BrowseDirection::Inverse, reference_type_id, true}, callback);
}

// Callback = void(std::vector<scada::Status> statuses, std::vector<NodeRef> nodes)
template<class Callback>
inline void RequestNodes(NodeRefService& service, const std::vector<scada::NodeId>& node_ids, const Callback& callback) {
  struct Result {
    Result(size_t count, const Callback& callback)
        : statuses{count, scada::StatusCode::Bad},
          nodes{count},
          callback{callback} {
    }

    ~Result() { callback(std::move(statuses), std::move(nodes)); }

    std::vector<scada::Status> statuses;
    std::vector<NodeRef> nodes;
    const Callback callback;
  };

  auto result = std::make_shared<Result>(node_ids.size(), callback);
  for (size_t i = 0; i < node_ids.size(); ++i) {
    service.GetNode(node_ids[i]).Fetch([result, i](NodeRef node) {
      result->statuses[i] = node.status();
      result->nodes[i] = node;
    });
  }
}

// Callback = void(const scada::Status& status, std::vector<NodeRef> nodes)
template<class Callback>
inline void BrowseNodes(NodeRefService& service,const  scada::BrowseDescription& description, const Callback& callback) {
  service.Browse(description, [&service, callback](const scada::Status& status, const scada::ReferenceDescriptions& references) {
    if (!status) {
      callback(status, {});
      return;
    }

    std::vector<scada::NodeId> node_ids;
    node_ids.reserve(references.size());
    for (auto& ref : references)
      node_ids.emplace_back(ref.node_id);

    RequestNodes(service, node_ids,
        [callback](const std::vector<scada::Status>&, std::vector<NodeRef> nodes) {
          nodes.erase(std::remove(nodes.begin(), nodes.end(), nullptr), nodes.end());
          callback(scada::StatusCode::Good, std::move(nodes));
        });
  });
}
