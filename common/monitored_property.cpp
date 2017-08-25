#include "common/monitored_property.h"

#include <cassert>

#include "core/standard_node_ids.h"

namespace {

scada::DataValue MakePropertyDataValue(const scada::DataValue& data_value,
                                       const scada::NodeId& prop_type_id) {
  // TODO:
  /*if (prop_type_id == scada::kQualifierProp) {
    auto result = data_value;
    result.value = !data_value.qualifier.general_bad();
    return result;
  }*/

  return scada::DataValue();
}

} // namespace

PropertyVariable::PropertyVariable(std::shared_ptr<scada::VariableHandle> base_variable,
                                   const scada::NodeId& prop_type_id)
    : monitored_item_(scada::CreateMonitoredVariable(std::move(base_variable))),
      prop_type_id_(prop_type_id) {
  assert(monitored_item_);

  monitored_item_->set_data_change_handler(
    [this](const scada::DataValue& data_value) {
      // WARNING: Current lambda can be deleted with |monitored_value_|.
      auto* self = this;
      if (data_value.qualifier.failed())
        self->monitored_item_.reset();
      self->ForwardData(MakePropertyDataValue(data_value, self->prop_type_id_));
    });
}

void PropertyVariable::Init() {
  monitored_item_->Subscribe();
}
