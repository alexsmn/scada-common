#include "timed_data/timed_data_service_impl.h"

#include "base/nested_logger.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
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
      null_timed_data_{std::make_shared<ErrorTimedData>(
          std::string{},
          base::WideToUTF16(kEmptyDisplayName))} {}

TimedDataServiceImpl::~TimedDataServiceImpl() {}

std::shared_ptr<TimedData> TimedDataServiceImpl::GetFormulaTimedData(
    base::StringPiece formula,
    const scada::AggregateFilter& aggregation) {
  auto expression = std::make_unique<ScadaExpression>();

  // May throw std::exception.
  try {
    expression->Parse(formula.as_string().c_str());
  } catch (const std::exception& e) {
    return std::make_shared<ErrorTimedData>(
        formula.as_string(),
        base::WideToUTF16(base::SysNativeMBToWide(e.what())));
  }

  std::shared_ptr<TimedData> data;

  std::string name;
  if (expression->IsSingleName(name)) {
    base::StringPiece unbraced_name = name;
    if (name.size() >= 2 && name[0] == '{' && name[name.size() - 1] == '}')
      unbraced_name = unbraced_name.substr(1, unbraced_name.size() - 2);

    return GetAliasTimedData(unbraced_name, aggregation);

  } else {
    std::vector<std::shared_ptr<TimedData>> operands(expression->items.size());
    for (size_t i = 0; i < operands.size(); ++i)
      operands[i] = GetAliasTimedData(expression->items[i].name, aggregation);
    auto logger = std::make_shared<NestedLogger>(logger_, formula.as_string());
    return std::make_shared<ExpressionTimedData>(
        std::move(expression), std::move(operands), std::move(logger));
  }
}

std::shared_ptr<TimedData> TimedDataServiceImpl::GetNodeTimedData(
    const scada::NodeId& node_id,
    const scada::AggregateFilter& aggregation) {
  if (node_id.is_null())
    return null_timed_data_;

  const auto cache_key = std::make_pair(node_id, aggregation);
  if (auto timed_data = node_id_cache_.Find(cache_key))
    return timed_data;

  auto node = node_service_.GetNode(node_id);
  if (!node)
    return null_timed_data_;

  auto& context = static_cast<TimedDataContext&>(*this);
  auto logger =
      std::make_shared<NestedLogger>(logger_, NodeIdToScadaString(node_id));
  auto timed_data = std::make_shared<TimedDataImpl>(
      node, std::move(aggregation), context, logger);

  node_id_cache_.Add(cache_key, timed_data);
  return timed_data;
}

std::shared_ptr<TimedData> TimedDataServiceImpl::GetAliasTimedData(
    base::StringPiece alias,
    const scada::AggregateFilter& aggregation) {
  auto node_id = NodeIdFromScadaString(alias);
  if (!node_id.is_null())
    return GetNodeTimedData(node_id, aggregation);

  const auto alias_string = alias.as_string();
  const auto cache_key = std::make_pair(alias_string, aggregation);
  if (auto timed_data = alias_cache_.Find(cache_key))
    return timed_data;

  auto timed_data = std::make_shared<AliasTimedData>(alias_string);

  auto weak_ptr = weak_ptr_factory_.GetWeakPtr();
  std::weak_ptr<AliasTimedData> weak_timed_data = timed_data;
  alias_resolver_(alias_string, [weak_ptr, weak_timed_data, alias_string,
                                 aggregation](const scada::Status& status,
                                              const scada::NodeId& node_id) {
    auto ptr = weak_ptr.get();
    auto timed_data = weak_timed_data.lock();
    if (!ptr || !timed_data)
      return;

    if (status) {
      timed_data->SetForwarded(ptr->GetNodeTimedData(node_id, aggregation));
    } else {
      timed_data->SetForwarded(
          std::make_shared<ErrorTimedData>(alias_string, ToString16(status)));
    }
  });

  alias_cache_.Add(cache_key, timed_data);
  return timed_data;
}
