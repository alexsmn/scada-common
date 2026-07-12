#pragma once

#include "node_service/node_model.h"
#include "node_service/node_service.h"

#include <memory>
#include <unordered_map>
#include <vector>

// NodeService test double that forwards every per-node operation to a
// registered NodeModel.
//
// `NodeRef` is now a `{NodeId, NodeService*}` cursor, so a test that wants a
// live node cursor needs a backing service. This double lets tests keep their
// lightweight per-node `MockNodeModel` stubs (ON_CALL/EXPECT_CALL on
// `Fetch`, `GetTargets`, `GetAttribute`, ...) and still obtain real cursors:
// register a model with `Add` and use the returned `NodeRef`. Operations on a
// cursor forward to the model registered under its node id; unregistered nodes
// answer with default-constructed values.
class ModelNodeService : public NodeService {
 public:
  // Registers |model| under |node_id| and returns a cursor to it.
  NodeRef Add(scada::NodeId node_id, std::shared_ptr<NodeModel> model) {
    models_[node_id] = std::move(model);
    return NodeRef{std::move(node_id), this};
  }

  // Registers |model| under the node id it reports via its NodeId attribute.
  NodeRef Add(std::shared_ptr<NodeModel> model) {
    scada::NodeId node_id =
        model->GetAttribute(scada::AttributeId::NodeId).as_node_id();
    return Add(std::move(node_id), std::move(model));
  }

  // Returns the model registered under |node_id|, or null.
  std::shared_ptr<NodeModel> GetModel(const scada::NodeId& node_id) const {
    auto i = models_.find(node_id);
    return i != models_.end() ? i->second : nullptr;
  }

  // NodeService:
  NodeRef GetNode(const scada::NodeId& node_id) override {
    return Find(node_id) ? NodeRef{node_id, this} : nullptr;
  }

  scada::Status GetStatus(const scada::NodeId& node_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetStatus() : scada::Status{scada::StatusCode::Good};
  }
  NodeFetchStatus GetFetchStatus(const scada::NodeId& node_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetFetchStatus() : NodeFetchStatus::None;
  }
  Awaitable<void> Fetch(const scada::NodeId& node_id,
                        const NodeFetchStatus& requested_status) override {
    if (NodeModel* model = Find(node_id))
      co_await model->Fetch(requested_status);
    co_return;
  }
  void StartFetch(const scada::NodeId& node_id,
                  const NodeFetchStatus& requested_status) override {
    if (NodeModel* model = Find(node_id))
      model->StartFetch(requested_status);
  }
  scada::Variant GetAttribute(const scada::NodeId& node_id,
                              scada::AttributeId attribute_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetAttribute(attribute_id) : scada::Variant{};
  }
  NodeRef GetDataType(const scada::NodeId& node_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetDataType() : nullptr;
  }
  NodeRef::Reference GetReference(const scada::NodeId& node_id,
                                  const scada::NodeId& reference_type_id,
                                  bool forward,
                                  const scada::NodeId& target_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetReference(reference_type_id, forward, target_id)
                 : NodeRef::Reference{};
  }
  std::vector<NodeRef::Reference> GetReferences(
      const scada::NodeId& node_id,
      const scada::NodeId& reference_type_id,
      bool forward) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetReferences(reference_type_id, forward)
                 : std::vector<NodeRef::Reference>{};
  }
  NodeRef GetTarget(const scada::NodeId& node_id,
                    const scada::NodeId& reference_type_id,
                    bool forward) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetTarget(reference_type_id, forward) : nullptr;
  }
  std::vector<NodeRef> GetTargets(const scada::NodeId& node_id,
                                  const scada::NodeId& reference_type_id,
                                  bool forward) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetTargets(reference_type_id, forward)
                 : std::vector<NodeRef>{};
  }
  NodeRef GetAggregate(const scada::NodeId& node_id,
                       const scada::NodeId& aggregate_declaration_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetAggregate(aggregate_declaration_id) : nullptr;
  }
  NodeRef GetChild(const scada::NodeId& node_id,
                   const scada::QualifiedName& child_name) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetChild(child_name) : nullptr;
  }
  scada::node GetScadaNode(const scada::NodeId& node_id) override {
    NodeModel* model = Find(node_id);
    return model ? model->GetScadaNode() : scada::node{};
  }

  boost::signals2::scoped_connection SubscribeModelChanged(
      const scada::NodeId&,
      const ModelChangedCallback&) override {
    return {};
  }
  boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const scada::NodeId&,
      const NodeSemanticChangedCallback&) override {
    return {};
  }
  boost::signals2::scoped_connection SubscribeNodeFetched(
      const scada::NodeId&,
      const NodeFetchedCallback&) override {
    return {};
  }
  boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const scada::NodeId&,
      const NodeStateChangedCallback&) override {
    return {};
  }

  boost::signals2::scoped_connection SubscribeModelChanged(
      const ModelChangedCallback&) const override {
    return {};
  }
  boost::signals2::scoped_connection SubscribeNodeSemanticChanged(
      const NodeSemanticChangedCallback&) const override {
    return {};
  }
  boost::signals2::scoped_connection SubscribeNodeFetched(
      const NodeFetchedCallback&) const override {
    return {};
  }
  boost::signals2::scoped_connection SubscribeNodeStateChanged(
      const NodeStateChangedCallback&) const override {
    return {};
  }

  size_t GetPendingTaskCount() const override { return 0; }

 private:
  NodeModel* Find(const scada::NodeId& node_id) const {
    auto i = models_.find(node_id);
    return i != models_.end() ? i->second.get() : nullptr;
  }

  std::unordered_map<scada::NodeId, std::shared_ptr<NodeModel>> models_;
};
