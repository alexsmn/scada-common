#include "address_space/variable.h"

#include "address_space/address_space.h"
#include "address_space/node_utils.h"
#include "address_space/node_variable_handle.h"
#include "address_space/type_definition.h"
#include "core/data_value.h"
#include "core/standard_node_ids.h"

namespace scada {

Variable::Variable() {}

Variable::~Variable() {
  assert(!variable_handle_.lock());
}

const DataType& Variable::GetDataType() const {
  auto* type = scada::AsVariableType(scada::GetTypeDefinition(*this));
  assert(type);
  return type->data_type();
}

std::shared_ptr<VariableHandle> Variable::GetVariableHandle() {
  auto variable_handle = variable_handle_.lock();
  if (!variable_handle) {
    variable_handle = std::make_shared<NodeVariableHandle>(*this);
    variable_handle->set_last_value(GetValue());
    variable_handle->set_last_change_time(GetChangeTime());
    variable_handle_ = variable_handle;
  }
  return variable_handle;
}

void Variable::Write(double value,
                     const NodeId& user_id,
                     const WriteFlags& flags,
                     const StatusCallback& callback) {
  callback(StatusCode::Bad);
}

void Variable::Call(const NodeId& method_id,
                    const std::vector<Variant>& arguments,
                    const scada::NodeId& user_id,
                    const StatusCallback& callback) {
  callback(StatusCode::Bad_WrongMethodId);
}

void Variable::HistoryReadRaw(base::Time from,
                              base::Time to,
                              const HistoryReadRawCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void Variable::HistoryReadEvents(base::Time from,
                                 base::Time to,
                                 const EventFilter& filter,
                                 const HistoryReadEventsCallback& callback) {
  callback(scada::StatusCode::Bad, {});
}

void Variable::Shutdown() {
  if (auto variable_handle = variable_handle_.lock())
    variable_handle->Deleted();
  variable_handle_.reset();
}

// GenericVariable

GenericVariable::GenericVariable(const DataType& data_type)
    : data_type_(data_type) {}

GenericVariable::GenericVariable(NodeId id,
                                 QualifiedName browse_name,
                                 scada::LocalizedText display_name,
                                 const DataType& data_type,
                                 Variant default_value)
    : data_type_(data_type), value_{std::move(default_value), {}, {}, {}} {
  set_id(std::move(id));
  SetBrowseName(std::move(browse_name));
  SetDisplayName(std::move(display_name));
}

void GenericVariable::SetValue(const DataValue& data_value) {
  if (value_ == data_value)
    return;

  bool is_current = IsUpdate(value_, data_value);
  if (is_current)
    value_ = data_value;

  if (auto variable_handle = variable_handle_.lock())
    variable_handle->ForwardData(data_value);
}

Variant GenericVariable::GetPropertyValue(const NodeId& prop_decl_id) const {
  auto i = properties_.find(prop_decl_id);
  return i == properties_.end() ? Variant() : i->second;
}

Status GenericVariable::SetPropertyValue(const NodeId& prop_decl_id,
                                         const Variant& value) {
  if (value.is_null()) {
    properties_.erase(prop_decl_id);
    return StatusCode::Good;
  }

  properties_[prop_decl_id] = value;
  return StatusCode::Good;
}

// DataVariable

DataVariable::DataVariable() {}

DataVariable::DataVariable(AddressSpace& address_space,
                           const NodeId& instance_declaration_id) {
  InitNode(address_space, instance_declaration_id);
}

bool DataVariable::InitNode(AddressSpace& address_space,
                            const NodeId& instance_declaration_id) {
  instance_declaration_ =
      AsVariable(address_space.GetNode(instance_declaration_id));
  if (!instance_declaration_)
    return false;

  auto* type = GetTypeDefinition(*instance_declaration_);
  if (!type)
    return false;

  value_ = instance_declaration_->GetValue();
  scada::AddReference(address_space, id::HasTypeDefinition, *this, *type);
  return true;
}

void DataVariable::SetValue(const DataValue& data_value) {
  if (value_ == data_value)
    return;

  bool is_current = IsUpdate(value_, data_value);
  if (is_current)
    value_ = data_value;

  if (auto variable_handle = variable_handle_.lock())
    variable_handle->ForwardData(data_value);
}

}  // namespace scada
