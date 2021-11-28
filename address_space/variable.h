#pragma once

#include "address_space/node.h"
#include "base/time/time.h"
#include "core/attribute_service.h"
#include "core/configuration_types.h"

#include <functional>
#include <map>
#include <memory>

namespace scada {

class AddressSpace;
class DataType;
class NodeVariableHandle;
class VariableHandle;
class VariableType;
class WriteFlags;
struct AggregateFilter;

using StatusCallback = std::function<void(Status&&)>;

class Variable : public Node {
 public:
  Variable();
  virtual ~Variable();

  // Can by any subtype of type definition's data-type.
  virtual const DataType& GetDataType() const;

  virtual DataValue GetValue() const = 0;
  virtual Status SetValue(const DataValue& data_value) = 0;

  virtual DateTime GetChangeTime() const = 0;

  virtual std::shared_ptr<VariableHandle> GetVariableHandle() const;

  virtual void Write(
      const std::shared_ptr<const scada::ServiceContext>& context,
      const scada::WriteValue& input,
      const scada::StatusCallback& callback);

  // TODO: Move to Object.
  virtual void Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback);

  // Node
  virtual NodeClass GetNodeClass() const override final {
    return NodeClass::Variable;
  }
  virtual void Shutdown() override;

 protected:
  mutable std::weak_ptr<NodeVariableHandle> variable_handle_;
};

class BaseVariable : public Variable {
 public:
  BaseVariable(const DataType& data_type);

  // Variable
  virtual const DataType& GetDataType() const override { return data_type_; }
  virtual DataValue GetValue() const override { return value_; }
  virtual Status SetValue(const DataValue& data_value) override;
  virtual DateTime GetChangeTime() const override { return change_time_; }

 protected:
  const DataType& data_type_;
  DataValue value_;
  DateTime change_time_;
};

class GenericVariable : public Variable {
 public:
  GenericVariable(const DataType& data_type);
  GenericVariable(NodeId id,
                  QualifiedName browse_name,
                  LocalizedText display_name,
                  const DataType& data_type,
                  Variant default_value = Variant());

  // Variable
  virtual const DataType& GetDataType() const override { return data_type_; }
  virtual DataValue GetValue() const override { return value_; }
  virtual Status SetValue(const DataValue& data_value) override;
  virtual DateTime GetChangeTime() const override { return change_time_; }

 protected:
  const DataType& data_type_;
  DataValue value_;
  DateTime change_time_;
};

}  // namespace scada
