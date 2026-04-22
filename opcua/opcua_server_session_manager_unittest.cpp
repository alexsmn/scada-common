#include "opcua/opcua_server_session_manager.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"

#include <gtest/gtest.h>

namespace opcua {
namespace {

class OpcUaServerSessionManagerTest : public testing::Test {
 protected:
  base::Time now_ = base::Time::Now();
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  OpcUaSessionManager MakeManager() {
    return OpcUaSessionManager{{
        .authenticator =
            [](scada::LocalizedText user_name,
               scada::LocalizedText password)
                -> Awaitable<scada::StatusOr<scada::AuthenticationResult>> {
              EXPECT_EQ(user_name, scada::LocalizedText{u"operator"});
              EXPECT_EQ(password, scada::LocalizedText{u"secret"});
              co_return scada::AuthenticationResult{
                  .user_id = scada::NodeId{42, 4}, .multi_sessions = true};
            },
        .now = [this] { return now_; },
        .min_timeout = base::TimeDelta::FromSeconds(10),
    }};
  }
};

TEST_F(OpcUaServerSessionManagerTest, ActivatesDetachesResumesAndClosesSession) {
  auto manager = MakeManager();

  const auto created = WaitAwaitable(executor_, manager.CreateSession({}));
  EXPECT_EQ(created.status.code(), scada::StatusCode::Good);
  EXPECT_FALSE(created.session_id.is_null());
  EXPECT_FALSE(created.authentication_token.is_null());

  const auto activated = WaitAwaitable(
      executor_,
      manager.ActivateSession({
          .session_id = created.session_id,
          .authentication_token = created.authentication_token,
          .user_name = scada::LocalizedText{u"operator"},
          .password = scada::LocalizedText{u"secret"},
      }));
  EXPECT_EQ(activated.status.code(), scada::StatusCode::Good);
  EXPECT_FALSE(activated.resumed);
  ASSERT_TRUE(activated.authentication_result.has_value());
  EXPECT_EQ(activated.authentication_result->user_id, scada::NodeId(42, 4));

  manager.DetachSession(created.authentication_token);
  const auto detached = manager.FindSession(created.authentication_token);
  ASSERT_TRUE(detached.has_value());
  EXPECT_FALSE(detached->attached);

  const auto resumed = WaitAwaitable(
      executor_,
      manager.ActivateSession({
          .session_id = created.session_id,
          .authentication_token = created.authentication_token,
      }));
  EXPECT_EQ(resumed.status.code(), scada::StatusCode::Good);
  EXPECT_TRUE(resumed.resumed);

  const auto closed = manager.CloseSession(
      {.session_id = created.session_id,
       .authentication_token = created.authentication_token});
  EXPECT_EQ(closed.status.code(), scada::StatusCode::Good);
  EXPECT_FALSE(manager.FindSession(created.authentication_token).has_value());
}

}  // namespace
}  // namespace opcua

