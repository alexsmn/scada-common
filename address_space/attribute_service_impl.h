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

class SyncAttributeServiceImpl : private AttributeServiceImplContext {
 public:
  explicit SyncAttributeServiceImpl(AttributeServiceImplContext&& context);

  scada::DataValue Read(const scada::ReadValueId& read_id);

 private:
  scada::DataValue ReadNode(const scada::Node& node,
                            scada::AttributeId attribute_id);
};

class AttributeServiceImpl : public scada::AttributeService {
 public:
  explicit AttributeServiceImpl(
      SyncAttributeServiceImpl& sync_attribute_service_impl);

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const std::vector<scada::WriteValueId>& value_ids,
                     const scada::NodeId& user_id,
                     const scada::WriteCallback& callback) override;

 private:
  SyncAttributeServiceImpl& sync_attribute_service_impl_;
};
