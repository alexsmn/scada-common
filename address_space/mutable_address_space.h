#pragma once

#include "address_space/address_space.h"
#include "common/node_state.h"
#include "core/node_attributes.h"

#include <memory>

namespace scada {
class ReferenceType;
}

class MutableAddressSpace : public scada::AddressSpace {
 public:
  virtual void AddNode(std::unique_ptr<scada::Node> node) = 0;

  virtual void DeleteNode(const scada::NodeId& id) = 0;

  // Returns false if nothing was changed.
  virtual bool ModifyNode(const scada::NodeId& id,
                          scada::NodeAttributes attributes,
                          scada::NodeProperties properties) = 0;

  virtual void AddReference(const scada::ReferenceType& type,
                            scada::Node& source,
                            scada::Node& target) = 0;

  virtual void DeleteReference(const scada::ReferenceType& type,
                               scada::Node& source,
                               scada::Node& target) = 0;
};
