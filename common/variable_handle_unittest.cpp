#include "common/variable_handle.h"

#include <gmock/gmock.h>

using namespace testing;

namespace scada {

class VariableHandleTest : public Test {
 protected:
  const std::shared_ptr<VariableHandle> variable_handle_ =
      std::make_shared<VariableHandle>();
  const std::shared_ptr<scada::MonitoredItem> monitored_item_ =
      CreateMonitoredVariable(variable_handle_);
  StrictMock<MockFunction<void(const scada::DataValue& data_value)>>
      data_change_handler_;
};

TEST_F(VariableHandleTest, UpdateQualifier_LastValueIsMissing) {
  monitored_item_->Subscribe(data_change_handler_.AsStdFunction());

  // |SourceTimestamp| is not updated, because next device value with a valid
  // |SourceTimestamp| overwrites this value.
  EXPECT_CALL(
      data_change_handler_,
      Call(FieldsAre(
          scada::Variant{}, scada::Qualifier{scada::Qualifier::OFFLINE},
          scada::Time{},
          ResultOf([](scada::Time t) { return scada::IsNull(t); }, IsFalse()),
          scada::StatusCode::Good)));

  variable_handle_->UpdateQualifier(0, scada::Qualifier::OFFLINE);
}

TEST_F(VariableHandleTest, UpdateQualifier_LastValueIsPresent) {
  const scada::Variant value{123};
  const auto server_timestamp = scada::Now();
  const auto source_timestamp = server_timestamp - std::chrono::seconds(10);

  const scada::DataValue last_data_value{value, scada::Qualifier{},
                                         source_timestamp, server_timestamp};
  variable_handle_->set_last_value(last_data_value);

  EXPECT_CALL(data_change_handler_, Call(last_data_value));

  monitored_item_->Subscribe(data_change_handler_.AsStdFunction());

  // |SourceTimestamp| is not updated, because next device value with a valid
  // |SourceTimestamp| overwrites this value.
  EXPECT_CALL(
      data_change_handler_,
      Call(FieldsAre(
          value, scada::Qualifier{scada::Qualifier::OFFLINE}, source_timestamp,
          ResultOf([](scada::Time t) { return scada::IsNull(t); }, IsFalse()),
          scada::StatusCode::Good)));

  variable_handle_->UpdateQualifier(0, scada::Qualifier::OFFLINE);
}
// A variable that never received data reports nothing to an internal
// subscriber — the device and formula chains must not see a sample nobody
// produced…
TEST_F(VariableHandleTest, NoFirstSampleBeforeAnyData) {
  monitored_item_->Subscribe(data_change_handler_.AsStdFunction());
}

// …but a client's item reports Bad_WaitingForInitialData at once, with no
// value and no source timestamp (OPC UA Part 4 §5.13.1.3 Monitoring mode,
// https://reference.opcfoundation.org/Core/Part4/v105/docs/5.13.1.3).
TEST_F(VariableHandleTest, AClientItemReportsAFirstSampleBeforeAnyData) {
  const std::shared_ptr<scada::MonitoredItem> client_item =
      CreateClientMonitoredVariable(variable_handle_);
  EXPECT_CALL(
      data_change_handler_,
      Call(FieldsAre(
          scada::Variant{}, scada::Qualifier{},
          ResultOf([](scada::Time t) { return scada::IsNull(t); }, IsTrue()),
          ResultOf([](scada::Time t) { return scada::IsNull(t); }, IsFalse()),
          scada::StatusCode::Bad_WaitingForInitialData)));
  client_item->Subscribe(data_change_handler_.AsStdFunction());

  // The first real reading follows as usual.
  const scada::DataValue reading{scada::Variant{1}, scada::Qualifier{},
                                 scada::Now(), scada::Now()};
  EXPECT_CALL(data_change_handler_, Call(reading));
  variable_handle_->ForwardData(reading);
}

// With data, a client's item reports the current value, as any item does.
TEST_F(VariableHandleTest, AClientItemReportsTheCurrentValue) {
  const scada::DataValue last{scada::Variant{7}, scada::Qualifier{},
                              scada::Now(), scada::Now()};
  variable_handle_->set_last_value(last);
  EXPECT_CALL(data_change_handler_, Call(last));
  CreateClientMonitoredVariable(variable_handle_)
      ->Subscribe(data_change_handler_.AsStdFunction());
}
}  // namespace scada
