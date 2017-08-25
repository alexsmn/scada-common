#include "common/monitored_expression.h"

#include "memdb/types.h"
#include "core/monitored_item_service.h"
#include "common/formula_util.h"
#include "common/timed_data/scada_expression.h"

std::unique_ptr<scada::MonitoredItem> CreateMonitoredExpression(scada::MonitoredItemService& monitored_item_service,
    const base::StringPiece& formula, scada::NodeId* single_node_id) {
  if (single_node_id)
    *single_node_id = {};

  std::unique_ptr<rt::ScadaExpression> expression(new rt::ScadaExpression);
  try {
    expression->Parse(formula.as_string().c_str());
  } catch (const std::exception&) {
    // logger().WriteF(LogSeverity::Warning, "Can't parse formula: %s", formula.as_string().c_str());
    return nullptr;
  }

  std::string string_id;
  if (expression->IsSingleName(string_id)) {
    scada::NodeId node_id;
    if (!IsNodeIdFormula(string_id, node_id))
      node_id = scada::NodeId::FromString(string_id);
    if (node_id.is_null()) {
      // logger().WriteF(LogSeverity::Warning, "Can't parse name: %s", string_id.c_str());
      return nullptr;
    }
    if (single_node_id)
      *single_node_id = node_id;
    return monitored_item_service.CreateMonitoredItem(node_id, OpcUa_Attributes_Value);
  }

  auto var = std::make_shared<MonitoredExpression>(monitored_item_service, std::move(expression));
  var->Init();
  return scada::CreateMonitoredVariable(std::move(var));
}


MonitoredExpression::MonitoredExpression(
    scada::MonitoredItemService& realtime_service,
    std::unique_ptr<rt::ScadaExpression> expression)
    : monitored_item_service_(realtime_service),
      expression_(std::move(expression)) {
}

void MonitoredExpression::Init() {
  auto& items = expression_->items;
  operands_.resize(items.size());
  for (size_t i = 0; i < items.size(); ++i) {
    auto& item = items[i];
    auto& operand = operands_[i];
    operand = monitored_item_service_.CreateMonitoredItem(scada::NodeId(item.name, 0), OpcUa_Attributes_Value);
    if (!operand) {
      Deleted();
      return;
    }
  }

  CalculateDataValue();

  for (size_t i = 0; i < operands_.size(); ++i) {
    operands_[i]->set_data_change_handler([this, i](const scada::DataValue& data_value) {
      // The functor and captured |this| can be deleted on operand reset.
      auto* self = this;
      auto index = i;
      if (data_value.qualifier.failed())
        operands_[i].reset();
      self->expression_->items[index].value = data_value;
      self->CalculateDataValue();
    });
    operands_[i]->Subscribe();
  }
}

void MonitoredExpression::CalculateDataValue() {
  auto now = base::Time::Now();
  auto value = expression_->Calculate();

  scada::Qualifier status_code;
  for (auto& item : expression_->items) {
    if (item.value.qualifier.general_bad()) {
      status_code.set_bad(true);
      break;
    }
  }

  ForwardData(scada::DataValue(std::move(value), status_code, now, now));
}