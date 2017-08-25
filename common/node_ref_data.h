#pragma once

#include <vector>

#include "common/node_ref.h"

struct NodeRefData {
  const scada::NodeId id;
  NodeRefService& node_service;
  NodeRef::References aggregates;
  // Instance-only.
  NodeRef type_definition;
  // Type-only.
  NodeRef supertype;
  // Forward non-hierarchical.
  NodeRef::References references;
  NodeRef data_type;
  bool fetched;
  scada::NodeClass node_class;
  std::string browse_name;
  base::string16 display_name;
  scada::DataValue data_value;
};

