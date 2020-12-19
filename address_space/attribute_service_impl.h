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

class SyncAttributeService {
 public:
  virtual ~SyncAttributeService() = default;

  virtual scada::DataValue Read(const scada::ReadValueId& read_id) = 0;
};

class SyncAttributeServiceImpl : private AttributeServiceImplContext,
                                 public SyncAttributeService {
 public:
  explicit SyncAttributeServiceImpl(AttributeServiceImplContext&& context);

  // SyncAttributeService
  virtual scada::DataValue Read(const scada::ReadValueId& read_id) override;

 private:
  scada::DataValue ReadNode(const scada::Node& node,
                            scada::AttributeId attribute_id);
};

class AttributeServiceImpl : public scada::AttributeService {
 public:
  explicit AttributeServiceImpl(SyncAttributeService& sync_attribute_service);

  // scada::AttributeService
  virtual void Read(const std::vector<scada::ReadValueId>& value_ids,
                    const scada::ReadCallback& callback) override;
  virtual void Write(const std::vector<scada::WriteValueId>& value_ids,
                     const scada::NodeId& user_id,
                     const scada::WriteCallback& callback) override;

 private:
  SyncAttributeService& sync_attribute_service_;
};
