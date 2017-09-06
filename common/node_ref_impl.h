#pragma once

#include <vector>

#include "common/node_ref.h"

class NodeRefService;

struct NodeRefImplReference {
  std::shared_ptr<NodeRefImpl> reference_type;
  std::shared_ptr<NodeRefImpl> target;
  bool forward;
};

class NodeRefImpl {
 public:
  NodeRefImpl(NodeRefService& service, scada::NodeId id) : service_{service}, id_{std::move(id)} {}
  virtual ~NodeRefImpl() {}

  scada::NodeId id() const { return id_; }

  std::vector<NodeRefImplReference> aggregates;
  // Instance-only.
  std::shared_ptr<NodeRefImpl> type_definition;
  // Type-only.
  std::shared_ptr<NodeRefImpl> supertype;
  // Forward non-hierarchical.
  std::vector<NodeRefImplReference> references;
  std::shared_ptr<NodeRefImpl> data_type;
  bool fetched;
  scada::NodeClass node_class;
  std::string browse_name;
  base::string16 display_name;
  scada::DataValue data_value;

 private:
  NodeRefService& service_;
  const scada::NodeId id_;
};
