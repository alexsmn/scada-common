#include "common/coroutine_session_proxy_notifier.h"

#include "base/test/test_executor.h"
#include "scada/session_service_mock.h"
#include "scada/status.h"

#include <boost/signals2/signal.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using testing::_;
using testing::Invoke;
using testing::StrictMock;

class ChannelProxy {
 public:
  void OnChannelOpened() { ++opened; }
  void OnChannelClosed() { ++closed; }

  int opened = 0;
  int closed = 0;
};

class UserChannelProxy {
 public:
  void OnChannelOpened(const scada::NodeId& user_id) {
    ++opened;
    last_user_id = user_id;
  }
  void OnChannelClosed() { ++closed; }

  int opened = 0;
  int closed = 0;
  scada::NodeId last_user_id;
};

}  // namespace

TEST(CoroutineSessionProxyNotifierTest,
     OpensImmediatelyWhenAdaptedSessionIsConnected) {
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockSessionService> session_service;
  scada::PromiseToCoroutineSessionServiceAdapter session_service_adapter{
      MakeTestAnyExecutor(executor), session_service};
  UserChannelProxy proxy;
  boost::signals2::signal<void(bool, const scada::Status&)>
      session_state_changed;

  EXPECT_CALL(session_service, SubscribeSessionStateChanged(_))
      .WillOnce(Invoke([&](const scada::SessionService::
                               SessionStateChangedCallback& callback) {
        return session_state_changed.connect(callback);
      }));
  EXPECT_CALL(session_service, IsConnected(_)).WillOnce(testing::Return(true));
  EXPECT_CALL(session_service, GetUserId())
      .WillOnce(testing::Return(scada::NodeId{1, 100}));

  CoroutineSessionProxyNotifier notifier{proxy, session_service_adapter};

  EXPECT_EQ(proxy.opened, 1);
  EXPECT_EQ(proxy.closed, 0);
  EXPECT_EQ(proxy.last_user_id, scada::NodeId(1, 100));
}

TEST(CoroutineSessionProxyNotifierTest,
     ForwardsSessionStateChangesThroughAdapter) {
  auto executor = std::make_shared<TestExecutor>();
  StrictMock<scada::MockSessionService> session_service;
  scada::PromiseToCoroutineSessionServiceAdapter session_service_adapter{
      MakeTestAnyExecutor(executor), session_service};
  ChannelProxy proxy;
  boost::signals2::signal<void(bool, const scada::Status&)>
      session_state_changed;

  EXPECT_CALL(session_service, SubscribeSessionStateChanged(_))
      .WillOnce(Invoke([&](const scada::SessionService::
                               SessionStateChangedCallback& callback) {
        return session_state_changed.connect(callback);
      }));
  EXPECT_CALL(session_service, IsConnected(_)).WillOnce(testing::Return(false));

  CoroutineSessionProxyNotifier notifier{proxy, session_service_adapter};

  session_state_changed(true, scada::Status{scada::StatusCode::Good});
  EXPECT_EQ(proxy.opened, 1);
  EXPECT_EQ(proxy.closed, 0);

  session_state_changed(false, scada::Status{scada::StatusCode::Bad});
  EXPECT_EQ(proxy.opened, 1);
  EXPECT_EQ(proxy.closed, 1);
}
