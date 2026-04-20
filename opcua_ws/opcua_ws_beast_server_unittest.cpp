#include "opcua_ws/opcua_ws_beast_server.h"

#include "opcua_ws/opcua_json_codec.h"

#include "base/asio_executor.h"
#include "scada/attribute_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/node_management_service_mock.h"
#include "scada/test/test_monitored_item.h"
#include "scada/view_service_mock.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/json/parse.hpp>
#include <boost/json/serialize.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <thread>

using namespace testing;

namespace opcua_ws {
namespace {

scada::NodeId NumericNode(scada::NumericId id, scada::NamespaceIndex ns = 2) {
  return {id, ns};
}

class TestMonitoredItemService : public scada::MonitoredItemService {
 public:
  std::shared_ptr<scada::MonitoredItem> CreateMonitoredItem(
      const scada::ReadValueId&,
      const scada::MonitoringParameters&) override {
    return std::make_shared<scada::TestMonitoredItem>();
  }
};

class BeastClient {
 public:
  void Connect(const std::string& host,
               unsigned short port,
               const std::string& origin,
               const std::string& subprotocol) {
    const auto results = resolver_.resolve(host, std::to_string(port));
    boost::asio::connect(websocket_.next_layer(), results);
    websocket_.set_option(boost::beast::websocket::stream_base::decorator(
        [origin, subprotocol](boost::beast::websocket::request_type& request) {
          if (!origin.empty())
            request.set(boost::beast::http::field::origin, origin);
          if (!subprotocol.empty()) {
            request.set(boost::beast::http::field::sec_websocket_protocol,
                        subprotocol);
          }
        }));
    websocket_.handshake(host + ":" + std::to_string(port), "/ua");
  }

  std::string Request(const OpcUaWsRequestMessage& request) {
    const auto payload = boost::json::serialize(EncodeJson(request));
    websocket_.text(true);
    websocket_.write(boost::asio::buffer(payload));

    boost::beast::flat_buffer buffer;
    websocket_.read(buffer);
    return boost::beast::buffers_to_string(buffer.data());
  }

  void Close() {
    if (websocket_.is_open())
      websocket_.close(boost::beast::websocket::close_code::normal);
  }

 private:
  boost::asio::io_context io_context_;
  boost::asio::ip::tcp::resolver resolver_{io_context_};
  boost::beast::websocket::stream<boost::asio::ip::tcp::socket> websocket_{
      io_context_};
};

class OpcUaWsBeastServerTest : public Test {
 protected:
  void SetUp() override {
    work_.emplace(boost::asio::make_work_guard(io_context_));
    thread_.emplace([this] { io_context_.run(); });
  }

  void TearDown() override {
    if (server_) {
      auto close_future = boost::asio::co_spawn(
          io_context_, server_->close(), boost::asio::use_future);
      EXPECT_EQ(close_future.get().value(), 0);
      server_.reset();
    }
    runtime_.reset();
    callback_executor_.reset();
    work_.reset();
    io_context_.stop();
    if (thread_)
      thread_->join();
  }

  void StartServer() {
    callback_executor_ =
        std::make_shared<AsioExecutor>(io_context_.get_executor());
    runtime_.emplace(OpcUaWsRuntimeContext{
        .executor = callback_executor_,
        .session_manager = session_manager_,
        .monitored_item_service = monitored_item_service_,
        .attribute_service = attribute_service_,
        .view_service = view_service_,
        .history_service = history_service_,
        .method_service = method_service_,
        .node_management_service = node_management_service_,
    });
    server_.emplace(OpcUaWsBeastServerContext{
        .executor = io_context_.get_executor(),
        .endpoint = {boost::asio::ip::make_address("127.0.0.1"), 0},
        .runtime = *runtime_,
        .allowed_origins = {"https://scada.local"},
    });
    auto open_future =
        boost::asio::co_spawn(io_context_, server_->open(), boost::asio::use_future);
    EXPECT_EQ(open_future.get(), transport::OK);
  }

  unsigned short port() const { return server_->local_endpoint().port(); }

  boost::asio::io_context io_context_;
  std::optional<boost::asio::executor_work_guard<
      boost::asio::io_context::executor_type>>
      work_;
  std::optional<std::jthread> thread_;

  NiceMock<scada::MockAttributeService> attribute_service_;
  NiceMock<scada::MockViewService> view_service_;
  NiceMock<scada::MockHistoryService> history_service_;
  NiceMock<scada::MockMethodService> method_service_;
  NiceMock<scada::MockNodeManagementService> node_management_service_;
  TestMonitoredItemService monitored_item_service_;
  std::shared_ptr<AsioExecutor> callback_executor_;
  OpcUaWsSessionManager session_manager_{{
      .authenticator =
          [](scada::LocalizedText,
             scada::LocalizedText)
              -> Awaitable<scada::StatusOr<scada::AuthenticationResult>> {
        co_return scada::AuthenticationResult{
            .user_id = NumericNode(700, 5), .multi_sessions = true};
      }}};
  std::optional<OpcUaWsRuntime> runtime_;
  std::optional<OpcUaWsBeastServer> server_;
};

TEST_F(OpcUaWsBeastServerTest,
       AcceptsValidHandshakeAndRoutesBrowsePagingEndToEnd) {
  StartServer();

  EXPECT_CALL(view_service_, Browse(_, _, _))
      .WillOnce(Invoke([&](const scada::ServiceContext& context,
                           const std::vector<scada::BrowseDescription>& inputs,
                           const scada::BrowseCallback& callback) {
        EXPECT_EQ(context.user_id(), NumericNode(700, 5));
        ASSERT_EQ(inputs.size(), 1u);
        callback(scada::StatusCode::Good,
                 {scada::BrowseResult{
                     .status_code = scada::StatusCode::Good,
                     .references = {{.reference_type_id = NumericNode(81),
                                     .forward = true,
                                     .node_id = NumericNode(82)},
                                    {.reference_type_id = NumericNode(83),
                                     .forward = true,
                                     .node_id = NumericNode(84)},
                                    {.reference_type_id = NumericNode(85),
                                     .forward = false,
                                     .node_id = NumericNode(86)}}}});
      }));

  BeastClient client;
  client.Connect("127.0.0.1", port(), "https://scada.local", "opcua+uajson");

  const auto create_response = DecodeResponseMessage(boost::json::parse(
      client.Request({.request_handle = 1, .body = OpcUaWsCreateSessionRequest{}})));
  const auto* created =
      std::get_if<OpcUaWsCreateSessionResponse>(&create_response.body);
  ASSERT_NE(created, nullptr);
  EXPECT_EQ(created->status.code(), scada::StatusCode::Good);

  const auto activate_response = DecodeResponseMessage(boost::json::parse(client.Request(
      {.request_handle = 2,
       .body =
           OpcUaWsActivateSessionRequest{
               .session_id = created->session_id,
               .authentication_token = created->authentication_token,
               .user_name = scada::LocalizedText{u"operator"},
               .password = scada::LocalizedText{u"secret"}}})));
  const auto* activated =
      std::get_if<OpcUaWsActivateSessionResponse>(&activate_response.body);
  ASSERT_NE(activated, nullptr);
  EXPECT_EQ(activated->status.code(), scada::StatusCode::Good);

  const auto browse_response = DecodeResponseMessage(boost::json::parse(client.Request(
      {.request_handle = 3,
       .body =
           BrowseRequest{
               .requested_max_references_per_node = 2,
               .inputs = {{.node_id = NumericNode(80),
                           .direction = scada::BrowseDirection::Both,
                           .reference_type_id = NumericNode(88),
                           .include_subtypes = true}}}})));
  const auto* browse = std::get_if<BrowseResponse>(&browse_response.body);
  ASSERT_NE(browse, nullptr);
  ASSERT_EQ(browse->results.size(), 1u);
  ASSERT_EQ(browse->results[0].references.size(), 2u);
  ASSERT_FALSE(browse->results[0].continuation_point.empty());

  const auto browse_next_response = DecodeResponseMessage(boost::json::parse(client.Request(
      {.request_handle = 4,
       .body =
           BrowseNextRequest{
               .continuation_points = {browse->results[0].continuation_point}}})));
  const auto* browse_next =
      std::get_if<BrowseNextResponse>(&browse_next_response.body);
  ASSERT_NE(browse_next, nullptr);
  ASSERT_EQ(browse_next->results.size(), 1u);
  ASSERT_EQ(browse_next->results[0].references.size(), 1u);
  EXPECT_EQ(browse_next->results[0].references[0].node_id, NumericNode(86));

  client.Close();
}

TEST_F(OpcUaWsBeastServerTest, RejectsOriginOutsideAllowList) {
  StartServer();

  BeastClient client;
  EXPECT_THROW(
      client.Connect("127.0.0.1", port(), "https://evil.local", "opcua+uajson"),
      boost::system::system_error);
}

TEST_F(OpcUaWsBeastServerTest, RejectsMissingRequiredSubprotocol) {
  StartServer();

  BeastClient client;
  EXPECT_THROW(client.Connect("127.0.0.1", port(), "https://scada.local", ""),
               boost::system::system_error);
}

}  // namespace
}  // namespace opcua_ws
