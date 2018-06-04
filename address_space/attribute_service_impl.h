#pragma once

#include "core/configuration_types.h"
#include "core/attribute_service.h"

namespace scada {
class AddressSpace;
class Node;
}

struct AttributeServiceImplContext {
  scada::AddressSpace& address_space_;
};

class AttributeServiceImpl : public scada::AttributeService,
                             private AttributeServiceImplContext {
 public:
  explicit AttributeServiceImpl(AttributeServiceImplContext&& context);

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const scada::NodeId& node_id, double value, const scada::NodeId& user_id,
                     const scada::WriteFlags& flags, const scada::StatusCallback& callback) override;

 private:
  scada::DataValue Read(const scada::ReadValueId& read_id, scada::AttributeService*& async_view_service);
  scada::DataValue ReadNode(const scada::Node& node, scada::AttributeId attribute_id);
  scada::DataValue ReadProperty(const scada::Node& node, base::StringPiece nested_name,
      scada::AttributeId attribute_id);
};
