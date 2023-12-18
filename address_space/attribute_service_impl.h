#pragma once

#include "common/sync_attribute_service.h"
#include "scada/attribute_service.h"

#include <span>

namespace scada {
class AddressSpace;
class Node;
}  // namespace scada

struct AttributeServiceImplContext {
  const scada::AddressSpace& address_space_;
};

class SyncAttributeServiceImpl : private AttributeServiceImplContext,
                                 public SyncAttributeService {
 public:
  explicit SyncAttributeServiceImpl(AttributeServiceImplContext&& context);

  // SyncAttributeService
  virtual std::vector<scada::DataValue> Read(
      const scada::ServiceContext& context,
      std::span<const scada::ReadValueId> inputs) override;
  virtual std::vector<scada::StatusCode> Write(
      const scada::ServiceContext& context,
      std::span<const scada::WriteValue> inputs) override;

 private:
  scada::DataValue Read(const scada::ReadValueId& input);
  scada::DataValue ReadNode(const scada::Node& node,
                            scada::AttributeId attribute_id);
};

class AttributeServiceImpl : public scada::AttributeService {
 public:
  explicit AttributeServiceImpl(SyncAttributeService& sync_attribute_service);

  // scada::AttributeService
  virtual void Read(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const std::shared_ptr<const std::vector<scada::ReadValueId>>& inputs,
      const scada::ReadCallback& callback) override;
  virtual void Write(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const std::shared_ptr<const std::vector<scada::WriteValue>>& inputs,
      const scada::WriteCallback& callback) override;

 private:
  SyncAttributeService& sync_attribute_service_;
};
