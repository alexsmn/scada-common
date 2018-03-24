#pragma once

#include <functional>
#include <map>
#include <memory>

#include "base/time/time.h"
#include "core/configuration_types.h"
#include "core/history_types.h"
#include "core/node.h"

namespace scada {

class Configuration;
class DataType;
class NodeVariableHandle;
class VariableHandle;
class VariableType;
class WriteFlags;

typedef std::function<void(const Status&)> StatusCallback;

class Variable : public Node {
 public:
  Variable();
  virtual ~Variable();

  // Can by any subtype of type definition's data-type.
  virtual const DataType& GetDataType() const;

  virtual DataValue GetValue() const = 0;
  virtual void SetValue(const DataValue& data_value) = 0;

  virtual base::Time GetChangeTime() const = 0;

  virtual std::shared_ptr<VariableHandle> GetVariableHandle();

  virtual void Write(double value,
                     const NodeId& user_id,
                     const WriteFlags& flags,
                     const StatusCallback& callback);

  // TODO: Move to Object.
  virtual void Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback);

  virtual void HistoryRead(scada::AttributeId attribute_id,
                           base::Time from,
                           base::Time to,
                           const Filter& filter,
                           const HistoryReadCallback& callback);

  // Node
  virtual NodeClass GetNodeClass() const override {
    return NodeClass::Variable;
  }
  virtual void Shutdown() override;

 protected:
  mutable std::weak_ptr<NodeVariableHandle> variable_handle_;
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
  virtual void SetValue(const DataValue& data_value) override;
  virtual base::Time GetChangeTime() const override { return change_time_; }
  virtual Variant GetPropertyValue(const NodeId& prop_decl_id) const override;
  virtual Status SetPropertyValue(const NodeId& prop_decl_id,
                                  const Variant& value) override;

 protected:
  const DataType& data_type_;
  DataValue value_;
  base::Time change_time_;
  std::map<NodeId, Variant> properties_;
};

class DataVariable : public Variable {
 public:
  DataVariable();
  DataVariable(Configuration& address_space,
               const NodeId& instance_declaration_id);

  bool InitNode(Configuration& address_space,
                const NodeId& instance_declaration_id);

  // Node
  virtual QualifiedName GetBrowseName() const override {
    return instance_declaration_->GetBrowseName();
  }
  virtual LocalizedText GetDisplayName() const override {
    return instance_declaration_->GetDisplayName();
  }

  // Variable
  virtual const DataType& GetDataType() const override {
    return instance_declaration_->GetDataType();
  }
  virtual DataValue GetValue() const override { return value_; }
  virtual void SetValue(const DataValue& data_value) override;
  virtual base::Time GetChangeTime() const override { return change_time_; }

 private:
  Variable* instance_declaration_ = nullptr;
  DataValue value_;
  base::Time change_time_;
};

class Property : public DataVariable {};

}  // namespace scada
