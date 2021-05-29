#pragma once

#include "core/node_id.h"

class TypeSystem {
 public:
  virtual ~TypeSystem() = default;

  virtual bool IsSubtypeOf(const scada::NodeId& type_definition_id,
                           const scada::NodeId& supertype_id) const = 0;
};
