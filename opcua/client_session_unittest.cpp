#include "opcua/client_session.h"

#include "base/awaitable_promise.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "net/net_executor_adapter.h"
#include "scada/status_exception.h"
#include "transport/transport_factory.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace {

using opcua::ClientSession;

class UnusedTransportFactory final : public transport::TransportFactory {
 public:
  transport::expected<transport::any_transport> CreateTransport(
      const transport::TransportString& /*transport_string*/,
      const transport::executor& /*executor*/,
      const transport::log_source& /*log*/) override {
    ADD_FAILURE() << "Unexpected transport construction";
    return transport::ERR_INVALID_ARGUMENT;
  }
};

class ClientSessionTest : public ::testing::Test {
 protected:
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  UnusedTransportFactory transport_factory_;
};

TEST_F(ClientSessionTest, InvalidEndpointRejectsAwaitableWithStatus) {
  auto session = std::make_shared<ClientSession>(executor_,
                                                     transport_factory_);

  try {
    WaitPromise(executor_,
                ToPromise(NetExecutorAdapter{executor_},
                          session->ConnectAsync(
                              {.connection_string = "http://host"})));
    FAIL() << "ConnectAsync unexpectedly succeeded";
  } catch (const scada::status_exception& e) {
    EXPECT_EQ(e.status().code(), scada::StatusCode::Bad);
  }
}

TEST_F(ClientSessionTest, InvalidEndpointRejectsLegacyPromiseWithStatus) {
  auto session = std::make_shared<ClientSession>(executor_,
                                                     transport_factory_);

  try {
    WaitPromise(executor_,
                session->Connect({.connection_string = "http://host"}));
    FAIL() << "Connect unexpectedly succeeded";
  } catch (const scada::status_exception& e) {
    EXPECT_EQ(e.status().code(), scada::StatusCode::Bad);
  }
}

TEST_F(ClientSessionTest, AwaitableServicesReportDisconnected) {
  auto session = std::make_shared<ClientSession>(executor_,
                                                     transport_factory_);

  auto read_inputs = std::make_shared<const std::vector<scada::ReadValueId>>(
      std::vector<scada::ReadValueId>{});
  auto [read_status, read_values] =
      WaitPromise(executor_,
                  ToPromise(NetExecutorAdapter{executor_},
                            session->Read({}, read_inputs)));
  EXPECT_EQ(read_status.code(), scada::StatusCode::Bad_Disconnected);
  EXPECT_TRUE(read_values.empty());

  auto write_inputs = std::make_shared<const std::vector<scada::WriteValue>>(
      std::vector<scada::WriteValue>{});
  auto [write_status, write_values] =
      WaitPromise(executor_,
                  ToPromise(NetExecutorAdapter{executor_},
                            session->Write({}, write_inputs)));
  EXPECT_EQ(write_status.code(), scada::StatusCode::Bad_Disconnected);
  EXPECT_TRUE(write_values.empty());

  auto [browse_status, browse_values] =
      WaitPromise(executor_,
                  ToPromise(NetExecutorAdapter{executor_},
                            session->Browse({}, {})));
  EXPECT_EQ(browse_status.code(), scada::StatusCode::Bad_Disconnected);
  EXPECT_TRUE(browse_values.empty());

  auto [translate_status, translate_values] =
      WaitPromise(executor_,
                  ToPromise(NetExecutorAdapter{executor_},
                            session->TranslateBrowsePaths({})));
  EXPECT_EQ(translate_status.code(), scada::StatusCode::Bad_Disconnected);
  EXPECT_TRUE(translate_values.empty());

  auto call_status =
      WaitPromise(executor_,
                  ToPromise(NetExecutorAdapter{executor_},
                            session->Call({}, {}, {}, {})));
  EXPECT_EQ(call_status.code(), scada::StatusCode::Bad_Disconnected);
}

TEST_F(ClientSessionTest, LegacyCallbacksUseAwaitableServices) {
  auto session = std::make_shared<ClientSession>(executor_,
                                                     transport_factory_);

  bool read_called = false;
  session->Read({}, std::make_shared<const std::vector<scada::ReadValueId>>(),
                [&](scada::Status status,
                    std::vector<scada::DataValue> values) {
                  read_called = true;
                  EXPECT_EQ(status.code(),
                            scada::StatusCode::Bad_Disconnected);
                  EXPECT_TRUE(values.empty());
                });
  Drain(executor_);
  EXPECT_TRUE(read_called);

  bool browse_called = false;
  session->Browse({}, {}, [&](scada::Status status,
                              std::vector<scada::BrowseResult> values) {
    browse_called = true;
    EXPECT_EQ(status.code(), scada::StatusCode::Bad_Disconnected);
    EXPECT_TRUE(values.empty());
  });
  Drain(executor_);
  EXPECT_TRUE(browse_called);

  bool call_called = false;
  session->Call({}, {}, {}, {}, [&](scada::Status status) {
    call_called = true;
    EXPECT_EQ(status.code(), scada::StatusCode::Bad_Disconnected);
  });
  Drain(executor_);
  EXPECT_TRUE(call_called);
}

}  // namespace
