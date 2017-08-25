#include "timed_data_spec.h"

#include "base/strings/sys_string_conversions.h"
#include "common/formula_util.h"
#include "common/scada_node_ids.h"
#include "timed_data/expression_timed_data.h"
#include "timed_data/scada_expression.h"
#include "timed_data/timed_data_service.h"
#include "common/node_ref_format.h"
#include "common/node_ref_util.h"

namespace {
const base::char16 kUnknownTitle[] = L"#ÈÌß?";
}

namespace rt {

TimedDataSpec::TimedDataSpec()
    : data_(nullptr),
      delegate_(nullptr),
      param(nullptr),
      from_(kTimedDataCurrentOnly) {
}

TimedDataSpec::TimedDataSpec(const TimedDataSpec& other)
     : data_(other.data_),
       formula_(other.formula_),
       delegate_(nullptr),
       from_(other.from_),
       param(nullptr) {
  if (data_)
    data_->Subscribe(*this);
}

TimedDataSpec::TimedDataSpec(std::shared_ptr<TimedData> data)
    : data_(std::move(data)),
      delegate_(nullptr),
      from_(kTimedDataCurrentOnly),
      param(nullptr) {
  data_->Subscribe(*this);
}

TimedDataSpec::~TimedDataSpec() {
  Reset();
}

void TimedDataSpec::SetData(std::shared_ptr<TimedData> data, bool update) {
  if (data == data_)
    return;

  if (data)
    data->Subscribe(*this);
  if (data_)
    data_->Unsubscribe(*this);
  data_ = data;
  
  if (data_ && update)
    data->SetFrom(from_);
}

void TimedDataSpec::Connect(TimedDataService& timed_data_service,
                            const std::string& formula) {
  formula_ = formula;

  std::unique_ptr<ScadaExpression> expression(new ScadaExpression);

  // May throw std::exception.
  try {
    expression->Parse(formula_.c_str());
  } catch (const std::exception& e) {
    throw std::runtime_error(base::StringPrintf("Wrong formula '%s': %s", formula.c_str(), e.what()));
  }

  std::shared_ptr<TimedData> data;

  std::string name;
  if (expression->IsSingleName(name)) {
    base::StringPiece unbraced_name = name;
    if (name.size() >= 2 && name[0] == '{' && name[name.size() - 1] == '}')
      unbraced_name = unbraced_name.substr(1, unbraced_name.size() - 2);

    const auto node_id = scada::NodeId::FromString(unbraced_name);
    if (node_id.is_null())
      throw std::runtime_error(base::StringPrintf("Wrong name '%s'", unbraced_name.as_string().c_str()));

    data = timed_data_service.GetNodeTimedData(node_id);
    // TODO: Handle.
    if (!data)
      throw std::runtime_error(base::StringPrintf("Wrong node id '%s'", node_id.ToString().c_str()));

  } else {
    // May throw std::exception too.
    data = std::make_shared<ExpressionTimedData>(timed_data_service, std::move(expression));
  }

  SetData(std::move(data), true);
}

void TimedDataSpec::Connect(TimedDataService& timed_data_service, const scada::NodeId& node_id) {
  formula_ = MakeNodeIdFormula(node_id);

  auto data = timed_data_service.GetNodeTimedData(node_id);
  // TODO: Handle.
  if (!data)
    throw std::runtime_error(base::StringPrintf("Wrong node id '%s'", node_id.ToString().c_str()));
  SetData(std::move(data), true);
}

void TimedDataSpec::SetFrom(base::Time from) {
  from_ = from;
  if (data_)
    data_->SetFrom(from);
}

base::string16 TimedDataSpec::GetCurrentString(int params) const {
  auto value = current();
  return GetValueString(value.value, value.qualifier, params);
}

base::string16 TimedDataSpec::GetValueString(const scada::Variant& value, scada::Qualifier qualifier, int params) const {
  if (data_) {
    if (auto node = GetNode())
      return FormatValue(node, value, qualifier, params);
  }

  std::string string_value;
  value.get(string_value);
  return base::SysNativeMBToWide(string_value);
}

bool TimedDataSpec::logical() const {
  // TODO: DataType.
  return IsInstanceOf(GetNode(), id::DiscreteItemType);
}

scada::NodeId TimedDataSpec::trid() const {
  return GetNode().id();
}

bool TimedDataSpec::ready() const {
  return data_ ? data_->ready_from() <= from_ : true;
}

TimedDataSpec& TimedDataSpec::operator =(const TimedDataSpec& other) {
  formula_ = other.formula_;
  from_ = other.from_;
  SetData(other.data_, false);
  return *this;
}

void TimedDataSpec::Write(double value, const scada::WriteFlags& flags,
                          const StatusCallback& callback) const {
  if (!data_) {
    if (callback)
      callback(scada::StatusCode::Bad_Disconnected);
    return;
  }

  return data_->Write(value, flags, callback);
}

void TimedDataSpec::Call(const scada::NodeId& method_id, const std::vector<scada::Variant>& arguments,
                         const StatusCallback& callback) const {
  if (!data_) {
    if (callback)
      callback(scada::StatusCode::Bad_Disconnected);
    return;
  }

  return data_->Call(method_id, arguments, callback);
}

void TimedDataSpec::Acknowledge() const {
  if (data_)
    data_->Acknowledge();
}

bool TimedDataSpec::alerting() const {
  return data_ && data_->alerting();
}

base::Time TimedDataSpec::ready_from() const {
  return data_ ? data_->ready_from() : kTimedDataCurrentOnly;
}

scada::DataValue TimedDataSpec::current() const {
  return data_ ? data_->current() : scada::DataValue();
}

base::Time TimedDataSpec::change_time() const {
  return data_ ? data_->change_time() : base::Time();
}

bool TimedDataSpec::historical() const {
  return from_ != kTimedDataCurrentOnly;
}

const TimedVQMap* TimedDataSpec::values() const {
  return data_ ? &data_->values() : nullptr;
}

NodeRef TimedDataSpec::GetNode() const {
  return data_ ? data_->GetNode() : NodeRef{};
}

base::string16 TimedDataSpec::GetTitle() const {
  return data_ ? data_->GetTitle() : kUnknownTitle;
}

scada::DataValue TimedDataSpec::GetValueAt(const base::Time& time) const {
  return data_ ? data_->GetValueAt(time) : scada::DataValue();
}

const events::EventSet* TimedDataSpec::GetEvents() const {
  return data_ ? data_->GetEvents() : nullptr;
}

bool TimedDataSpec::operator==(const TimedDataSpec& other) const {
  return data_ == other.data_;
}

} // namespace rt
