#include "timed_data/timed_data_service_impl.h"

#include "base/strings/sys_string_conversions.h"
#include "common/node_id_util.h"
#include "common/node_service.h"
#include "timed_data/alias_timed_data.h"
#include "timed_data/error_timed_data.h"
#include "timed_data/expression_timed_data.h"
#include "timed_data/scada_expression.h"
#include "timed_data/timed_data_impl.h"

template <class T>
bool IsTimedCacheExpired(const T& value) {
  return value.use_count() <= 1;
}

// TimedDataServiceImpl

TimedDataServiceImpl::TimedDataServiceImpl(TimedDataContext&& context,
                                           std::shared_ptr<const Logger> logger)
    : TimedDataContext{std::move(context)},
      logger_{std::move(logger)},
      node_id_cache_{io_context_},
      alias_cache_{io_context_},
      null_timed_data_{
          std::make_shared<ErrorTimedData>(std::string{}, kEmptyDisplayName)} {}

TimedDataServiceImpl::~TimedDataServiceImpl() {}

std::shared_ptr<rt::TimedData> TimedDataServiceImpl::GetFormulaTimedData(
    base::StringPiece formula) {
  auto expression = std::make_unique<rt::ScadaExpression>();

  // May throw std::exception.
  try {
    expression->Parse(formula.as_string().c_str());
  } catch (const std::exception& e) {
    return std::make_shared<ErrorTimedData>(formula.as_string(),
                                            base::SysNativeMBToWide(e.what()));
  }

  std::shared_ptr<rt::TimedData> data;

  std::string name;
  if (expression->IsSingleName(name)) {
    base::StringPiece unbraced_name = name;
    if (name.size() >= 2 && name[0] == '{' && name[name.size() - 1] == '}')
      unbraced_name = unbraced_name.substr(1, unbraced_name.size() - 2);

    return GetAliasTimedData(unbraced_name);

  } else {
    std::vector<std::shared_ptr<rt::TimedData>> operands(
        expression->items.size());
    for (size_t i = 0; i < operands.size(); ++i)
      operands[i] = GetAliasTimedData(expression->items[i].name);
    return std::make_shared<rt::ExpressionTimedData>(std::move(expression),
                                                     std::move(operands));
  }
}

std::shared_ptr<rt::TimedData> TimedDataServiceImpl::GetNodeTimedData(
    const scada::NodeId& node_id) {
  if (node_id.is_null())
    return null_timed_data_;

  if (auto timed_data = node_id_cache_.Find(node_id))
    return timed_data;

  auto node = node_service_.GetNode(node_id);
  if (!node)
    return null_timed_data_;

  auto& context = static_cast<TimedDataContext&>(*this);
  auto logger =
      std::make_shared<NestedLogger>(logger_, NodeIdToScadaString(node_id));
  auto timed_data = std::make_shared<rt::TimedDataImpl>(node, context, logger);

  node_id_cache_.Add(node_id, timed_data);
  return timed_data;
}

std::shared_ptr<rt::TimedData> TimedDataServiceImpl::GetAliasTimedData(
    base::StringPiece alias) {
  auto node_id = NodeIdFromScadaString(alias);
  if (!node_id.is_null())
    return GetNodeTimedData(node_id);

  auto alias_string = alias.as_string();
  if (auto timed_data = alias_cache_.Find(alias_string))
    return timed_data;

  auto timed_data = std::make_shared<AliasTimedData>(alias_string);

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  std::weak_ptr<AliasTimedData> weak_timed_data = timed_data;
  alias_resolver_(alias_string, [weak_ptr, weak_timed_data, alias_string](
                                    const scada::Status& status,
                                    const scada::NodeId& node_id) {
    auto ptr = weak_ptr.get();
    auto timed_data = weak_timed_data.lock();
    if (!ptr || !timed_data)
      return;

    if (status) {
      timed_data->SetForwarded(ptr->GetNodeTimedData(node_id));
    } else {
      timed_data->SetForwarded(
          std::make_shared<ErrorTimedData>(alias_string, ToString16(status)));
    }
  });

  alias_cache_.Add(alias_string, timed_data);
  return timed_data;
}
