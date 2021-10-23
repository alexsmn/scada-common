#include "address_space/variable_handle.h"

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
          scada::DateTime{}, Property(&scada::DateTime::is_null, IsFalse()),
          scada::StatusCode::Good)));

  variable_handle_->UpdateQualifier(0, scada::Qualifier::OFFLINE);
}

TEST_F(VariableHandleTest, UpdateQualifier_LastValueIsPresent) {
  const scada::Variant value{123};
  const auto server_timestamp = scada::DateTime::Now();
  const auto source_timestamp =
      server_timestamp - scada::Duration::FromSeconds(10);

  const scada::DataValue last_data_value{value, scada::Qualifier{},
                                         source_timestamp, server_timestamp};
  variable_handle_->set_last_value(last_data_value);

  EXPECT_CALL(data_change_handler_, Call(last_data_value));

  monitored_item_->Subscribe(data_change_handler_.AsStdFunction());

  // |SourceTimestamp| is not updated, because next device value with a valid
  // |SourceTimestamp| overwrites this value.
  EXPECT_CALL(data_change_handler_,
              Call(FieldsAre(value, scada::Qualifier{scada::Qualifier::OFFLINE},
                             source_timestamp,
                             Property(&scada::DateTime::is_null, IsFalse()),
                             scada::StatusCode::Good)));

  variable_handle_->UpdateQualifier(0, scada::Qualifier::OFFLINE);
}
}  // namespace scada
