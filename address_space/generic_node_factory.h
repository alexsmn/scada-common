#pragma once

#include "address_space/node_factory.h"

namespace scada {
class NodeId;
}  // namespace scada

class MutableAddressSpace;

class GenericNodeFactory final : public NodeFactory {
 public:
  // `create_components` materializes the type's HasComponent
  // InstanceDeclarations (data variables, child Objects, Methods) onto each
  // instance, recursively. It defaults to OFF because turning it on changes
  // what a generic Browse returns: every placeholder InstanceDeclaration in the
  // static model would grow DeviceType's data variables, UserType's and
  // RoleType's Methods and Iec61850DeviceType's Model object. Callers that want
  // a fully materialized instance opt in.
  explicit GenericNodeFactory(MutableAddressSpace& address_space,
                              bool create_properties = true,
                              bool create_components = false)
      : address_space_{address_space},
        create_properties_{create_properties},
        create_components_{create_components} {}

  virtual std::pair<scada::Status, scada::Node*> CreateNode(
      const scada::NodeState& node_state) override;

 private:
  std::pair<scada::Status, scada::Node*> CreateNodeHelper(
      const scada::NodeState& node_state,
      const scada::NodeId& parent_id);

  MutableAddressSpace& address_space_;
  const bool create_properties_ = false;
  const bool create_components_ = false;
  // Component materialization recurses back through CreateNode, so a type
  // whose component is typed by itself would not terminate. Bound it.
  int component_depth_ = 0;
  static constexpr int kMaxComponentDepth = 8;
};
