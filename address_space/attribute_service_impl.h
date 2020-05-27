#pragma once

#include "core/attribute_service.h"
#include "core/configuration_types.h"

namespace scada {
class AddressSpace;
class Node;
}  // namespace scada

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
  virtual void Write(const std::vector<scada::WriteValueId>& value_ids,
                     const scada::NodeId& user_id,
                     const scada::WriteCallback& callback) override;

 private:
  scada::DataValue Read(const scada::ReadValueId& read_id);
  scada::DataValue ReadNode(const scada::Node& node,
                            scada::AttributeId attribute_id);

  scada::Status WriteNode(const scada::Node& node,
                          const scada::WriteValue& value);
};
