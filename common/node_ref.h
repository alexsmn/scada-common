#pragma once

#include "common/node_fetch_status.h"
#include "core/attribute_ids.h"
#include "core/data_value.h"
#include "core/node_class.h"
#include "core/standard_node_ids.h"
#include "core/write_flags.h"

#include <functional>
#include <memory>
#include <optional>

namespace scada {
class MonitoredItem;
struct MonitoringParameters;
using StatusCallback = std::function<void(Status&&)>;
}  // namespace scada

class NodeModel;
class NodeRefObserver;

class NodeRef {
 public:
  constexpr NodeRef() noexcept {}
  constexpr NodeRef(std::nullptr_t) noexcept {}
  template <class T>
  constexpr NodeRef(std::shared_ptr<T> model) noexcept
      : model_{std::move(model)} {}

  scada::Status status() const;

  bool fetched() const;
  bool children_fetched() const;

  void Fetch(const NodeFetchStatus& requested_status) const;

  using FetchCallback = std::function<void(const NodeRef& node)>;
  void Fetch(const NodeFetchStatus& requested_status,
             const FetchCallback& callback) const;

  scada::Variant attribute(scada::AttributeId attribute_id) const;

  scada::NodeId node_id() const;
  std::optional<scada::NodeClass> node_class() const;
  scada::QualifiedName browse_name() const;
  scada::LocalizedText display_name() const;
  scada::Variant value() const;
  NodeRef data_type() const;

  NodeRef type_definition() const;
  NodeRef supertype() const;
  NodeRef parent() const;

  NodeRef target(const scada::NodeId& reference_type_id) const;
  NodeRef inverse_target(const scada::NodeId& reference_type_id) const;
  std::vector<NodeRef> targets(
      const scada::NodeId& reference_type_id = scada::id::References) const;
  std::vector<NodeRef> inverse_targets(
      const scada::NodeId& reference_type_id = scada::id::References) const;

  struct Reference;
  using References = std::vector<Reference>;

  // Non-hierarchical references.
  Reference reference(const scada::NodeId& reference_type_id) const;
  Reference inverse_reference(const scada::NodeId& reference_type_id) const;
  std::vector<Reference> references(
      const scada::NodeId& reference_type_id = scada::id::References) const;
  std::vector<Reference> inverse_references(
      const scada::NodeId& reference_type_id = scada::id::References) const;

  NodeRef operator[](const scada::QualifiedName& child_name) const;
  NodeRef operator[](const scada::NodeId& aggregate_declaration_id) const;

  explicit operator bool() const noexcept { return model_ != nullptr; }

  bool operator==(const NodeRef& other) const {
    return node_id() == other.node_id();
  }
  bool operator!=(const NodeRef& other) const { return !operator==(other); }

  bool operator<(const NodeRef& other) const {
    return node_id() < other.node_id();
  }

  const std::shared_ptr<const NodeModel>& model() const { return model_; }

  void Subscribe(NodeRefObserver& observer) const;
  void Unsubscribe(NodeRefObserver& observer) const;

  std::unique_ptr<scada::MonitoredItem> CreateMonitoredItem(
      scada::AttributeId attribute_id,
      const scada::MonitoringParameters& params) const;

  using ReadCallback = std::function<void(scada::DataValue&& value)>;
  void Read(scada::AttributeId attribute_id,
            const ReadCallback& callback) const;

  void Write(scada::AttributeId attribute_id,
             const scada::Variant& value,
             const scada::WriteFlags& flags,
             const scada::NodeId& user_id,
             const scada::StatusCallback& callback) const;

  void Call(const scada::NodeId& method_id,
            const std::vector<scada::Variant>& arguments,
            const scada::NodeId& user_id,
            const scada::StatusCallback& callback) const;

 private:
  std::shared_ptr<const NodeModel> model_;
};

struct NodeRef::Reference {
  NodeRef reference_type;
  NodeRef target;
  bool forward;
};
