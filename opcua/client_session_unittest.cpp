#include "opcua/client_session.h"

#include "base/awaitable_promise.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "net/net_executor_adapter.h"
#include "opcua/binary/secure_channel.h"
#include "opcua/binary/service_codec.h"
#include "scada/monitored_item.h"
#include "scada/monitoring_parameters.h"
#include "scada/status_exception.h"
#include "transport/transport_factory.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <deque>
#include <memory>
#include <string>
#include <utility>
#include <variant>
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

struct ScriptedState {
  std::deque<std::string> incoming;
  std::vector<std::string> writes;
  bool opened = false;
  bool closed = false;
};

class ScriptedTransport {
 public:
  ScriptedTransport(transport::executor executor,
                    std::shared_ptr<ScriptedState> state)
      : executor_{std::move(executor)}, state_{std::move(state)} {}
  ScriptedTransport(ScriptedTransport&&) = default;
  ScriptedTransport& operator=(ScriptedTransport&&) = default;
  ScriptedTransport(const ScriptedTransport&) = delete;
  ScriptedTransport& operator=(const ScriptedTransport&) = delete;

  transport::awaitable<transport::error_code> open() {
    state_->opened = true;
    co_return transport::OK;
  }

  transport::awaitable<transport::error_code> close() {
    state_->closed = true;
    co_return transport::OK;
  }

  transport::awaitable<transport::expected<transport::any_transport>> accept() {
    co_return transport::ERR_NOT_IMPLEMENTED;
  }

  transport::awaitable<transport::expected<size_t>> read(std::span<char> data) {
    if (state_->incoming.empty()) {
      co_return size_t{0};
    }
    auto chunk = std::move(state_->incoming.front());
    state_->incoming.pop_front();
    if (chunk.size() > data.size()) {
      co_return transport::ERR_INVALID_ARGUMENT;
    }
    std::ranges::copy(chunk, data.begin());
    co_return chunk.size();
  }

  transport::awaitable<transport::expected<size_t>> write(
      std::span<const char> data) {
    state_->writes.emplace_back(data.begin(), data.end());
    co_return data.size();
  }

  std::string name() const { return "ScriptedTransport"; }
  bool message_oriented() const { return false; }
  bool connected() const { return state_->opened && !state_->closed; }
  bool active() const { return true; }
  transport::executor get_executor() { return executor_; }

 private:
  transport::executor executor_;
  std::shared_ptr<ScriptedState> state_;
};

class ScriptedTransportFactory final : public transport::TransportFactory {
 public:
  explicit ScriptedTransportFactory(std::shared_ptr<ScriptedState> state)
      : state_{std::move(state)} {}

  transport::expected<transport::any_transport> CreateTransport(
      const transport::TransportString& /*transport_string*/,
      const transport::executor& executor,
      const transport::log_source& /*log*/) override {
    return transport::any_transport{ScriptedTransport{executor, state_}};
  }

 private:
  std::shared_ptr<ScriptedState> state_;
};

std::string AsString(const std::vector<char>& bytes) {
  return {bytes.begin(), bytes.end()};
}

constexpr std::uint32_t kChannelId = 42;
constexpr std::uint32_t kTokenId = 1;

std::vector<char> BuildOpenResponseFrame() {
  const opcua::binary::OpenSecureChannelResponse response{
      .response_header = {.request_handle = 1,
                          .service_result = scada::StatusCode::Good},
      .server_protocol_version = 0,
      .security_token = {.channel_id = kChannelId,
                         .token_id = kTokenId,
                         .created_at = 0,
                         .revised_lifetime = 60000},
      .server_nonce = {},
  };
  const opcua::binary::SecureConversationMessage message{
      .frame_header = {.message_type = opcua::binary::MessageType::SecureOpen,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = kChannelId,
      .asymmetric_security_header = opcua::binary::AsymmetricSecurityHeader{
          .security_policy_uri = std::string{opcua::binary::kSecurityPolicyNone},
          .sender_certificate = {},
          .receiver_certificate_thumbprint = {},
      },
      .sequence_header = {.sequence_number = 1, .request_id = 1},
      .body = opcua::binary::EncodeOpenSecureChannelResponseBody(response),
  };
  return opcua::binary::EncodeSecureConversationMessage(message);
}

std::vector<char> BuildServiceResponseFrame(std::uint32_t request_id,
                                            std::uint32_t request_handle,
                                            opcua::ResponseBody body) {
  const auto encoded =
      opcua::binary::EncodeServiceResponse(request_handle, std::move(body));
  const opcua::binary::SecureConversationMessage message{
      .frame_header = {.message_type = opcua::binary::MessageType::SecureMessage,
                       .chunk_type = 'F',
                       .message_size = 0},
      .secure_channel_id = kChannelId,
      .asymmetric_security_header = std::nullopt,
      .symmetric_security_header =
          opcua::binary::SymmetricSecurityHeader{.token_id = kTokenId},
      .sequence_header = {.sequence_number = request_id + 1,
                          .request_id = request_id},
      .body = encoded.value(),
  };
  return opcua::binary::EncodeSecureConversationMessage(message);
}

void PrimeConnectAndOpen(const std::shared_ptr<ScriptedState>& state) {
  state->incoming.push_back(AsString(opcua::binary::EncodeAcknowledgeMessage(
      {.receive_buffer_size = 65535, .send_buffer_size = 65535})));
  state->incoming.push_back(AsString(BuildOpenResponseFrame()));
}

void PrimeSessionEstablishment(const std::shared_ptr<ScriptedState>& state) {
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/2, /*request_handle=*/1,
      opcua::ResponseBody{opcua::CreateSessionResponse{
          .status = scada::StatusCode::Good,
          .session_id = scada::NodeId{111},
          .authentication_token = scada::NodeId{222},
          .server_nonce = scada::ByteString{},
          .revised_timeout = base::TimeDelta::FromSeconds(60),
      }})));
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/3, /*request_handle=*/2,
      opcua::ResponseBody{opcua::ActivateSessionResponse{
          .status = scada::StatusCode::Good}})));
}

void PrimeSubscriptionCreation(const std::shared_ptr<ScriptedState>& state) {
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/4, /*request_handle=*/3,
      opcua::ResponseBody{opcua::CreateSubscriptionResponse{
          .status = scada::StatusCode::Good,
          .subscription_id = 77,
          .revised_publishing_interval_ms = 500.0,
          .revised_lifetime_count = 1200,
          .revised_max_keep_alive_count = 20}})));
  state->incoming.push_back(AsString(BuildServiceResponseFrame(
      /*request_id=*/5, /*request_handle=*/4,
      opcua::ResponseBody{opcua::CreateMonitoredItemsResponse{
          .status = scada::StatusCode::Good,
          .results = {opcua::MonitoredItemCreateResult{
              .status = scada::StatusCode::Good,
              .monitored_item_id = 101,
              .revised_sampling_interval_ms = 250.0,
              .revised_queue_size = 1,
          }}}})));
}

std::vector<opcua::RequestBody> DecodeServiceRequests(
    const std::vector<std::string>& writes) {
  std::vector<opcua::RequestBody> requests;
  for (const auto& write : writes) {
    const std::vector<char> bytes{write.begin(), write.end()};
    auto message = opcua::binary::DecodeSecureConversationMessage(bytes);
    if (!message) {
      continue;
    }
    auto request = opcua::binary::DecodeServiceRequest(message->body);
    if (request) {
      requests.push_back(std::move(request->body));
    }
  }
  return requests;
}

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

  bool write_called = false;
  session->Write({}, std::make_shared<const std::vector<scada::WriteValue>>(),
                 [&](scada::Status status,
                     std::vector<scada::StatusCode> values) {
                   write_called = true;
                   EXPECT_EQ(status.code(),
                             scada::StatusCode::Bad_Disconnected);
                   EXPECT_TRUE(values.empty());
                 });
  Drain(executor_);
  EXPECT_TRUE(write_called);

  bool browse_called = false;
  session->Browse({}, {}, [&](scada::Status status,
                              std::vector<scada::BrowseResult> values) {
    browse_called = true;
    EXPECT_EQ(status.code(), scada::StatusCode::Bad_Disconnected);
    EXPECT_TRUE(values.empty());
  });
  Drain(executor_);
  EXPECT_TRUE(browse_called);

  bool translate_called = false;
  session->TranslateBrowsePaths(
      {}, [&](scada::Status status,
              std::vector<scada::BrowsePathResult> values) {
        translate_called = true;
        EXPECT_EQ(status.code(), scada::StatusCode::Bad_Disconnected);
        EXPECT_TRUE(values.empty());
      });
  Drain(executor_);
  EXPECT_TRUE(translate_called);

  bool call_called = false;
  session->Call({}, {}, {}, {}, [&](scada::Status status) {
    call_called = true;
    EXPECT_EQ(status.code(), scada::StatusCode::Bad_Disconnected);
  });
  Drain(executor_);
  EXPECT_TRUE(call_called);
}

TEST_F(ClientSessionTest, LegacyCallbacksAreDroppedAfterSessionDestroy) {
  auto session = std::make_shared<ClientSession>(executor_,
                                                     transport_factory_);

  bool read_called = false;
  bool write_called = false;
  bool browse_called = false;
  bool translate_called = false;
  bool call_called = false;

  session->Read({}, std::make_shared<const std::vector<scada::ReadValueId>>(),
                [&](scada::Status, std::vector<scada::DataValue>) {
                  read_called = true;
                });
  session->Write({}, std::make_shared<const std::vector<scada::WriteValue>>(),
                 [&](scada::Status, std::vector<scada::StatusCode>) {
                   write_called = true;
                 });
  session->Browse({}, {}, [&](scada::Status,
                              std::vector<scada::BrowseResult>) {
    browse_called = true;
  });
  session->TranslateBrowsePaths(
      {}, [&](scada::Status, std::vector<scada::BrowsePathResult>) {
        translate_called = true;
      });
  session->Call({}, {}, {}, {}, [&](scada::Status) {
    call_called = true;
  });

  session.reset();
  Drain(executor_);

  EXPECT_FALSE(read_called);
  EXPECT_FALSE(write_called);
  EXPECT_FALSE(browse_called);
  EXPECT_FALSE(translate_called);
  EXPECT_FALSE(call_called);
}

TEST_F(ClientSessionTest, MonitoredItemUsesSubscriptionCoroutineTasks) {
  auto state = std::make_shared<ScriptedState>();
  PrimeConnectAndOpen(state);
  PrimeSessionEstablishment(state);
  PrimeSubscriptionCreation(state);

  ScriptedTransportFactory transport_factory{state};
  auto session = std::make_shared<ClientSession>(executor_,
                                                 transport_factory);

  ASSERT_NO_THROW(
      WaitPromise(executor_, session->Connect({.host = "localhost:4840"})));

  auto monitored_item = session->CreateMonitoredItem(
      scada::ReadValueId{.node_id = scada::NodeId{1},
                         .attribute_id = scada::AttributeId::Value},
      scada::MonitoringParameters{
          .sampling_interval = base::TimeDelta::FromMilliseconds(250),
          .queue_size = 1});
  monitored_item->Subscribe(static_cast<scada::DataChangeHandler>(
      [](scada::DataValue) {}));
  Drain(executor_);

  monitored_item.reset();
  Drain(executor_);

  const auto requests = DecodeServiceRequests(state->writes);
  EXPECT_NE(std::ranges::find_if(requests,
                                 [](const opcua::RequestBody& body) {
                                   return std::holds_alternative<
                                       opcua::CreateSubscriptionRequest>(body);
                                 }),
            requests.end());
  EXPECT_NE(std::ranges::find_if(requests,
                                 [](const opcua::RequestBody& body) {
                                   return std::holds_alternative<
                                       opcua::CreateMonitoredItemsRequest>(body);
                                 }),
            requests.end());
  EXPECT_NE(std::ranges::find_if(requests,
                                 [](const opcua::RequestBody& body) {
                                   return std::holds_alternative<
                                       opcua::DeleteMonitoredItemsRequest>(body);
                                 }),
            requests.end());
}

}  // namespace
